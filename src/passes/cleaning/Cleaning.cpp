#include "omvll/log.hpp"
#include "omvll/utils.hpp"
#include "omvll/passes/cleaning/Cleaning.hpp"
#include "omvll/omvll_config.hpp"

#include <llvm/Demangle/Demangle.h>

using namespace llvm;

namespace omvll {

PreservedAnalyses Cleaning::run(Module &M, ModuleAnalysisManager &FAM) {
  if (!config.cleaning)
    return PreservedAnalyses::all();

  SINFO("[{}] Executing on module {}", name(), M.getName());
  bool Changed = false;
  for (Function& F : M) {
    std::string Name  = demangle(F.getName().str());
    StringRef NRef = Name;
    if (NRef.startswith("_JNIEnv::") && config.inline_jni_wrappers) {
      SDEBUG("[{}] Inlining {}", Name);
      F.addFnAttr(Attribute::AlwaysInline);
      Changed = true;
    }
  }

  if (config.shuffle_functions) {
    shuffleFunctions(M);
    Changed = true;
  }

  SINFO("[{}] Changes{}applied on module {}", name(), Changed ? " " : " not ",
        M.getName());

  return Changed ? PreservedAnalyses::none() :
                   PreservedAnalyses::all();

}
}

