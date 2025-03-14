//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/MemoryBuffer.h"

#include "omvll/ObfuscationConfig.hpp"
#include "omvll/PyConfig.hpp"
#include "omvll/jitter.hpp"
#include "omvll/log.hpp"
#include "omvll/passes/anti-hook/AntiHook.hpp"
#include "omvll/utils.hpp"

using namespace llvm;

namespace omvll {

// Versions of Frida (as of 16.0.2) fail to hook functions that begin with
// instructions using x16/x17 registers. These stubs are using these registers
// and are injected in the prologue of the function to protect.

struct PrologueInfoTy {
  std::string Asm;
  size_t Size;
};

// clang-format off
static const std::vector<PrologueInfoTy> AntiFridaPrologues = {
  {R"delim(
    mov x17, x17;
    mov x16, x16;
  )delim", 2},

  {R"delim(
    mov x16, x16;
    mov x17, x17;
  )delim", 2}
};
// clang-format on

bool AntiHook::runOnFunction(Function &F) {
  if (F.getInstructionCount() == 0)
    return false;

  const auto &TT = Triple(F.getParent()->getTargetTriple());
  if (!TT.isAArch64()) {
    SWARN("[{}] Only AArch64 target is supported. Skipping...", name());
    return false;
  }

  if (F.hasPrologueData())
    fatalError("Cannot inject a hooking prologue in the function " +
               demangle(F.getName().str()) + " since there is one.");

  SDEBUG("[{}] Injecting Anti-Frida prologue in {}", name(), F.getName());

  std::uniform_int_distribution<size_t> Dist(0, AntiFridaPrologues.size() - 1);
  size_t Idx = Dist(*RNG);
  const PrologueInfoTy &P = AntiFridaPrologues[Idx];

  std::unique_ptr<MemoryBuffer> Insts = JIT->jitAsm(P.Asm, P.Size);
  if (!Insts)
    fatalError("Cannot JIT Anti-Frida prologue: \n" + P.Asm);

  auto *Int8Ty = Type::getInt8Ty(F.getContext());
  auto *Prologue = ConstantDataVector::getRaw(Insts->getBuffer(),
                                              Insts->getBufferSize(), Int8Ty);
  F.setPrologueData(Prologue);

  return true;
}

PreservedAnalyses AntiHook::run(Module &M, ModuleAnalysisManager &FAM) {
  if (isModuleGloballyExcluded(&M)) {
    SINFO("Excluding module [{}]", M.getName());
    return PreservedAnalyses::all();
  }

  bool Changed = false;
  PyConfig &Config = PyConfig::instance();
  SINFO("[{}] Executing on module {}", name(), M.getName());
  JIT = std::make_unique<Jitter>(M.getTargetTriple());
  RNG = M.createRNG(name());

  for (Function &F : M) {
    if (isFunctionGloballyExcluded(&F) ||
        !Config.getUserConfig()->antiHooking(F.getParent(), &F))
      continue;

    Changed |= runOnFunction(F);
  }

  SINFO("[{}] Changes {} applied on module {}", name(), Changed ? "" : "not",
        M.getName());

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // end namespace omvll
