#include "omvll/Jitter.hpp"
#include "omvll/utils.hpp"
#include "omvll/log.hpp"

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/DiagnosticOptions.h>
#include <clang/Basic/FileManager.h>
#include <clang/Basic/FileSystemOptions.h>
#include <clang/Basic/LangOptions.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/TargetInfo.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Driver/Compilation.h>
#include <clang/Driver/Driver.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Lex/HeaderSearch.h>
#include <clang/Lex/HeaderSearchOptions.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Lex/PreprocessorOptions.h>
#include <clang/Parse/ParseAST.h>
#include <clang/Sema/Sema.h>

#include <llvm/AsmParser/Parser.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/PassManager.h>
#include <llvm/InitializePasses.h>
#include <llvm/Object/ELFObjectFile.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/Cloning.h>

using namespace clang;
using namespace llvm;

namespace omvll {

Jitter::Jitter(const std::string& Triple):
  Triple_{Triple},
  Clang_{new CompilerInstance{}},
  DiagID_{new DiagnosticIDs{}},
  DiagOpts_{new DiagnosticOptions{}},
  VFS_{new vfs::InMemoryFileSystem{}},
  Ctx_{new LLVMContext{}}
{

  InitializeAllTargetInfos();
  InitializeAllTargets();
  InitializeAllTargetMCs();
  InitializeAllTargetMCAs();
  InitializeAllAsmPrinters();

  InitializeNativeTargetAsmParser();

  /* =======================================================================================
   * This part of the code is highly inspired from the project
   * DragonFFI: https://github.com/aguinet/dragonffi
   * Developed by Adrien Guinet™
   * =======================================================================================
   */

  auto *DiagClient = new TextDiagnosticPrinter{errs(), &*DiagOpts_};
  Diags_ = new DiagnosticsEngine{DiagID_, &*DiagOpts_, DiagClient, true};

  static const char *EmptyFiles[] = {
    "/foo.c",
    "/bin/omvll"
  };
  IntrusiveRefCntPtr<vfs::OverlayFileSystem> Overlay(new vfs::OverlayFileSystem{vfs::getRealFileSystem()});

  for (const char* Path : EmptyFiles) {
    if (StringRef(Path) == "/foo.c") {
      VFS_->addFile(Path, 0, MemoryBuffer::getMemBuffer(R"delim(
      int main() { return 0; }
      )delim"));
    } else {
      VFS_->addFile(Path, 0, MemoryBuffer::getMemBuffer("\n"));
    }
  }
  Overlay->pushOverlay(VFS_);

  Driver_ = std::make_unique<driver::Driver>("/bin/omvll", Triple, *Diags_,
                                             "omvll JIT Compiler", Overlay);
  Driver_->setCheckInputsExist(false);

  std::unique_ptr<driver::Compilation> CP(Driver_->BuildCompilation({"-fsyntax-only",  "/foo.c"}));

  if (!CP) {
    fatalError("Can't instantiate Clang's driver");
  }

  const driver::JobList &Jobs = CP->getJobs();

  if (Jobs.empty() || !isa<driver::Command>(*Jobs.begin())) {
    SmallString<256> Msg;
    llvm::raw_svector_ostream OS(Msg);
    Jobs.Print(OS, "; ", true);
    fatalError(OS.str().str().c_str());
  }

  const driver::Command &Cmd = cast<driver::Command>(*Jobs.begin());

  // Initialize a compiler invocation object from the clang (-cc1) arguments.
  const llvm::opt::ArgStringList &CCArgs = Cmd.getArguments();

  std::unique_ptr<CompilerInvocation> CI(new CompilerInvocation{});

  CompilerInvocation::CreateFromArgs(*CI, CCArgs, *Diags_);

  clang::TargetOptions& TO = CI->getTargetOpts();
  {
    TO.Triple     = Triple;
    TO.HostTriple = llvm::sys::getProcessTriple();
  }

  CodeGenOptions& CGO = CI->getCodeGenOpts();
  {
    CGO.OptimizeSize      = false;
    CGO.OptimizationLevel = 0;
    CGO.CodeModel         = "default";
    CGO.RelocationModel   = Reloc::PIC_;
  }

  DiagnosticOptions& DO = CI->getDiagnosticOpts();
  {
    DO.ShowCarets = true;
  }

  LangOptions& LO = *CI->getLangOpts();
  {
    LO.LineComment      = true;
    LO.Optimize         = false;
    LO.C99              = true;
    LO.C11              = true;
    LO.CPlusPlus        = false;
    LO.CPlusPlus11      = false;
    LO.CPlusPlus14      = false;
    LO.CPlusPlus17      = false;
    LO.CPlusPlus20      = false;
    LO.CXXExceptions    = false;
    LO.CXXOperatorNames = false;
    LO.Bool             = false;
    LO.WChar            = false; // builtin in C++, typedef in C (stddef.h)
    LO.EmitAllDecls     = true;

    LO.MSVCCompat       = false;
    LO.MicrosoftExt     = false;
    LO.AsmBlocks        = false;
    LO.DeclSpecKeyword  = false;
    LO.MSBitfields      = false;

    LO.GNUMode          = false;
    LO.GNUKeywords      = false;
    LO.GNUAsm           = false;
  }

  FrontendOptions& FO = CI->getFrontendOpts();
  {
    FO.ProgramAction = frontend::EmitLLVMOnly;
  }

  /* HeaderSearchOptions& HSO = */ CI->getHeaderSearchOpts();
  {
    /*
     * In the future O-MVLL could welcome user-defined include dir
     * to support #include in the Jitted code
     */
  }

  Clang_->setInvocation(std::move(CI));
  Clang_->setDiagnostics(&*Diags_);

  FileMgr_ = new FileManager(Clang_->getFileSystemOpts(), std::move(Overlay));
  Clang_->createSourceManager(*FileMgr_);
  Clang_->setFileManager(&*FileMgr_);
}

Jitter::Jitter() : Jitter(sys::getProcessTriple())
{

}

std::unique_ptr<Jitter> Jitter::Create(const std::string& Triple) {
  return std::unique_ptr<Jitter>(new Jitter(Triple));
}

std::unique_ptr<Jitter> Jitter::Create() {
  return std::unique_ptr<Jitter>(new Jitter());
}


std::unique_ptr<llvm::Module> Jitter::generate(const std::string& code) {
  return generate(code, *Ctx_);
}

std::unique_ptr<llvm::Module> Jitter::generate(const std::string& code, LLVMContext& Ctx) {
  auto Buf = MemoryBuffer::getMemBufferCopy(code);
  Clang_->getInvocation().getFrontendOpts().Inputs.clear();
  Clang_->getInvocation().getFrontendOpts().Inputs.push_back(
    FrontendInputFile(*Buf, InputKind(Language::C), false));

  std::unique_ptr<CodeGenAction> action = std::make_unique<EmitLLVMOnlyAction>(&Ctx);
  if (!Clang_->ExecuteAction(*action)) {
    fatalError("Can't compile code");
  }
  return action->takeModule();
}

std::unique_ptr<llvm::orc::LLJIT> Jitter::compile(llvm::Module& M) {
  static ExitOnError ExitOnErr;
  auto JITB = ExitOnErr(orc::LLJITBuilder().create());

  std::unique_ptr<llvm::Module> Mod = CloneModule(M);

  auto Context = std::make_unique<LLVMContext>();
  ExitOnErr(JITB->addIRModule(llvm::orc::ThreadSafeModule(std::move(Mod), std::move(Context))));
  return JITB;
}

size_t Jitter::getFunctionSize(llvm::object::ObjectFile& Obj, llvm::StringRef Name) {
  if (auto* ELF = dyn_cast<object::ELFObjectFileBase>(&Obj)) {
    for (const object::ELFSymbolRef& Sym : ELF->symbols()) {
      if (Sym.getELFType() != ELF::STT_FUNC) {
        continue;
      }
      if (auto Str = Sym.getName()) {
        if (*Str == Name) {
          return Sym.getSize();
        }
      }
    }
    fatalError("jitAsm: Unable to find symbol '" + Name.str() + "'");
  }
  //if (auto* MachO = dyn_cast<object::MachOObjectFile>(&Obj)) {
  //  for (const object::SymbolRef Sym : MachO->symbols()) {
  //    object::DataRefImpl Impl = Sym.getRawDataRefImpl();
  //    if (auto SName = MachO->getSymbolName(Impl)) {
  //      if (*SName != Name) {
  //        continue;
  //      }
  //      if (auto ItSec = MachO->getSymbolSection(Impl)) {
  //        uint64_t size = (*ItSec)->getSize();
  //      }
  //      //return
  //    }
  //  }
  //  fatalError("jitAsm: Unable to find symbol '" + Name.str() + "'");
  //}
  fatalError("jitAsm: Unsupported file format");
}

std::unique_ptr<MemoryBuffer> Jitter::jitAsm(const std::string& Asm, size_t Size) {
  static constexpr const char FNAME[] = "__omvll_asm_func";

  static ExitOnError ExitOnErr;

  auto Context = std::make_unique<LLVMContext>();
  auto M = std::make_unique<llvm::Module>("__omvll_asm_jit", *Context);

  Function *F = Function::Create(llvm::FunctionType::get(llvm::Type::getVoidTy(*Context), {}, false),
                                 Function::ExternalLinkage, FNAME, M.get());

  llvm::BasicBlock *BB = llvm::BasicBlock::Create(*Context, "EntryBlock", F);
  IRBuilder<> builder(BB);

  auto* FType = llvm::FunctionType::get(builder.getVoidTy(), false);
  InlineAsm* rawAsm = InlineAsm::get(FType, Asm, "",
    /* hasSideEffects */ true,
    /* isStackAligned */ true
  );

  builder.CreateCall(FType, rawAsm);
  builder.CreateRetVoid();
  orc::LLJITBuilder Builder;
  std::string TT = Triple_;
  orc::JITTargetMachineBuilder JTMB{llvm::Triple(TT)};
  // Note(romain): At this point, the code crashes on OSX/iOS.
  // -> To be investigated
#if 0
  JTMB.setRelocationModel(Reloc::Model::PIC_);
  JTMB.setCodeModel(CodeModel::Large);
  JTMB.setCodeGenOptLevel(CodeGenOpt::Level::None);

  Builder
    .setPlatformSetUp(orc::setUpInactivePlatform) // /!\Only for iOS???
    .setJITTargetMachineBuilder(JTMB);
#endif

  Builder
    .setJITTargetMachineBuilder(std::move(JTMB));

  std::unique_ptr<orc::LLJIT> JITB = ExitOnErr(Builder.create());

  if (!JITB) {
    fatalError("JITB is null");
  }

  M->setDataLayout(JITB->getDataLayout());

  auto& IRC = JITB->getIRCompileLayer();
  orc::IRCompileLayer::IRCompiler& Compiler = IRC.getCompiler();
  if (auto Res = Compiler(*M)) {
    std::unique_ptr<llvm::MemoryBuffer> Buff = std::move(*Res);
    size_t FunSize = Size * /* Sizeof AArch64 inst=*/ 4;

    //if (auto E = object::ObjectFile::createObjectFile(*Buff)) {
    //  std::unique_ptr<object::ObjectFile> Obj = std::move(E.get());
    //  FunSize = getFunctionSize(*Obj, FNAME);
    //}

    if (FunSize == 0) {
      fatalError("Can't get the function size");
    }

    ExitOnErr(JITB->addObjectFile(std::move(Buff)));

    if (auto L = JITB->lookup(FNAME)) {
      uint64_t Addr = L->getAddress();
      auto* ptr = reinterpret_cast<const char*>(Addr);
      return MemoryBuffer::getMemBufferCopy({ptr, FunSize});
    }
  }
  fatalError("Can't compile: " + Asm);
}

}
