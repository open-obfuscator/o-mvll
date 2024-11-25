//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "llvm/AsmParser/Parser.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/ThreadSafeModule.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Object/ELFObjectFile.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "omvll/jitter.hpp"
#include "omvll/utils.hpp"

using namespace llvm;

static constexpr auto AsmFunctionName = "__omvll_asm_func";

namespace llvm {
namespace object {
class MachOObjectFile;
} // end namespace object
} // end namespace llvm

namespace omvll {

Jitter::Jitter(const std::string &Triple)
    : Triple{Triple}, Ctx{new LLVMContext{}} {
  InitializeNativeTarget();
  InitializeNativeTargetAsmParser();
  InitializeNativeTargetAsmPrinter();
}

std::unique_ptr<orc::LLJIT> Jitter::compile(Module &M) {
  static ExitOnError ExitOnErr;
  auto JITB = ExitOnErr(orc::LLJITBuilder().create());
  std::unique_ptr<Module> ClonedM = CloneModule(M);
  auto Context = std::make_unique<LLVMContext>();

  ExitOnErr(JITB->addIRModule(
      orc::ThreadSafeModule(std::move(ClonedM), std::move(Context))));
  return JITB;
}

size_t Jitter::getFunctionSize(object::ObjectFile &Obj, StringRef Name) {
  if (auto *ELF = dyn_cast<object::ELFObjectFileBase>(&Obj)) {
    for (const object::ELFSymbolRef &Sym : ELF->symbols()) {
      if (Sym.getELFType() != ELF::STT_FUNC)
        continue;
      if (auto Str = Sym.getName())
        if (*Str == Name)
          return Sym.getSize();
    }
    fatalError("jitAsm: Unable to find symbol " + Name.str());
  }

#if 0
  if (auto *MachO = dyn_cast<object::MachOObjectFile>(&Obj)) {
    for (const object::SymbolRef Sym : MachO->symbols()) {
      object::DataRefImpl Impl = Sym.getRawDataRefImpl();
      if (auto SName = MachO->getSymbolName(Impl)) {
        if (*SName != Name)
          continue;
      }
      if (auto ItSec = MachO->getSymbolSection(Impl)) {
        uint64_t Size = (*ItSec)->getSize();
        return Size;
      }
    }
    fatalError("jitAsm: Unable to find symbol '" + Name.str() + "'");
  }
#endif

  fatalError("jitAsm: Unsupported file format");
}

std::unique_ptr<MemoryBuffer> Jitter::jitAsm(const std::string &Asm,
                                             size_t Size) {
  static ExitOnError ExitOnErr;

  auto Context = std::make_unique<LLVMContext>();
  auto M = std::make_unique<Module>("__omvll_asm_jit", *Context);

  Function *F =
      Function::Create(FunctionType::get(Type::getVoidTy(*Context), {}, false),
                       Function::ExternalLinkage, AsmFunctionName, M.get());

  BasicBlock *BB = BasicBlock::Create(*Context, "EntryBlock", F);
  IRBuilder<> IRB(BB);

  auto *FType = FunctionType::get(IRB.getVoidTy(), false);
  InlineAsm *RawAsm = InlineAsm::get(FType, Asm, "", /* hasSideEffects */ true,
                                     /* isStackAligned */ true);
  IRB.CreateCall(FType, RawAsm);
  IRB.CreateRetVoid();

  orc::LLJITBuilder Builder;
  std::string TT = Triple;
  orc::JITTargetMachineBuilder JTMB{llvm::Triple(TT)};
  JTMB.setRelocationModel(Reloc::Model::PIC_);
  JTMB.setCodeModel(CodeModel::Large);
  JTMB.setCodeGenOptLevel(CodeGenOpt::Level::None);

  Builder.setPlatformSetUp(orc::setUpInactivePlatform)
      .setJITTargetMachineBuilder(JTMB);
  Builder.setJITTargetMachineBuilder(std::move(JTMB));

  std::unique_ptr<orc::LLJIT> JITB = ExitOnErr(Builder.create());
  if (!JITB)
    fatalError("JITB is null");

  M->setDataLayout(JITB->getDataLayout());

  auto &IRC = JITB->getIRCompileLayer();
  orc::IRCompileLayer::IRCompiler &Compiler = IRC.getCompiler();
  if (auto Res = Compiler(*M)) {
    std::unique_ptr<MemoryBuffer> Buff = std::move(*Res);
    size_t FunSize = Size * /* Sizeof AArch64 inst=*/4;

#if 0
    if (auto E = object::ObjectFile::createObjectFile(*Buff)) {
      std::unique_ptr<object::ObjectFile> Obj = std::move(E.get());
      FunSize = getFunctionSize(*Obj, AsmFunctionName);
    }
#endif

    if (FunSize == 0)
      fatalError("Cannot retrieve function size");

    ExitOnErr(JITB->addObjectFile(std::move(Buff)));
    if (auto L = JITB->lookup(AsmFunctionName)) {
      auto *Ptr = reinterpret_cast<const char *>(L->getValue());
      return MemoryBuffer::getMemBufferCopy({Ptr, FunSize});
    }
  }

  fatalError("Cannot compile " + Asm);
}

} // end namespace omvll
