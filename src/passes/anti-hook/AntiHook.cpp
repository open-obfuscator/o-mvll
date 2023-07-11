#include "omvll/passes/anti-hook/AntiHook.hpp"
#include "omvll/log.hpp"
#include "omvll/utils.hpp"
#include "omvll/PyConfig.hpp"
#include "omvll/passes/Metadata.hpp"
#include "omvll/ObfuscationConfig.hpp"
#include "omvll/Jitter.hpp"

#include <llvm/Demangle/Demangle.h>
#include <llvm/IR/Constants.h>
#include <llvm/Support/MemoryBuffer.h>

using namespace llvm;

namespace omvll {

/*
 * The current versions of Frida (as of 16.0.2) fail to hook functions
 * that begin with instructions using x16/x17 registers.
 *
 * These stubs are using these registers and are injected in the prologue of the
 * function to protect
 */
struct PrologueInfoTy {
  std::string Asm;
  size_t size;
};

static const std::vector<PrologueInfoTy> ANTI_FRIDA_PROLOGUES = {
  {R"delim(
    mov x17, x17;
    mov x16, x16;
  )delim", 2},

  {R"delim(
    mov x16, x16;
    mov x17, x17;
  )delim", 2}
};

bool AntiHook::runOnFunction(llvm::Function &F) {
  if (F.getInstructionCount() == 0) {
    return false;
  }

  PyConfig& config = PyConfig::instance();
  if (!config.getUserConfig()->anti_hooking(F.getParent(), &F)) {
    return false;
  }

  if (F.hasPrologueData()) {
    fatalError("Can't inject a hooking prologue in the function '" + demangle(F.getName().str()) + "' "
               "since there is one.");
  }

  std::uniform_int_distribution<size_t> Dist(0, ANTI_FRIDA_PROLOGUES.size() - 1);
  size_t idx = Dist(*RNG_);
  const PrologueInfoTy& P = ANTI_FRIDA_PROLOGUES[idx];

  std::unique_ptr<MemoryBuffer> insts = jitter_->jitAsm(P.Asm, P.size);

  if (insts == nullptr) {
    fatalError("Can't JIT Anti-Frida prologue: \n" + P.Asm);
  }

  auto* Int8Ty = Type::getInt8Ty(F.getContext());
  auto* Prologue = ConstantDataVector::getRaw(insts->getBuffer(), insts->getBufferSize(), Int8Ty);
  F.setPrologueData(Prologue);

  return true;
}

PreservedAnalyses AntiHook::run(Module &M,
                                ModuleAnalysisManager &FAM) {
  bool Changed = false;
  jitter_ = Jitter::Create(M.getTargetTriple());

  RNG_ = M.createRNG(name());

  for (Function& F : M) {
    Changed |= runOnFunction(F);
  }

  SINFO("[{}] Done!", name());
  return Changed ? PreservedAnalyses::none() :
                   PreservedAnalyses::all();

}
}

