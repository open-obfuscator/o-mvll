#include "omvll/Jitter.hpp"

#include "omvll/utils.hpp"
#include "omvll/log.hpp"

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
#include <llvm/IRReader/IRReader.h>
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

using namespace llvm;

namespace omvll {

Jitter::Jitter(const std::string &Triple)
    : Triple_{Triple}, Ctx_{new LLVMContext{}} {
  InitializeNativeTarget();
  InitializeNativeTargetAsmParser();
  InitializeNativeTargetAsmPrinter();
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
  JTMB.setRelocationModel(Reloc::Model::PIC_);
  JTMB.setCodeModel(CodeModel::Large);
  JTMB.setCodeGenOptLevel(CodeGenOpt::Level::None);

  Builder
    .setPlatformSetUp(orc::setUpInactivePlatform) // /!\Only for iOS???
    .setJITTargetMachineBuilder(JTMB);

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
