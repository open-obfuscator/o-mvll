//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "llvm/Demangle/Demangle.h"

#include "omvll/log.hpp"
#include "omvll/utils.hpp"
#include "omvll/passes/cleaning/Cleaning.hpp"
#include "omvll/omvll_config.hpp"

using namespace llvm;

namespace omvll {

PreservedAnalyses Cleaning::run(Module &M, ModuleAnalysisManager &FAM) {
  if (!Config.Cleaning)
    return PreservedAnalyses::all();

  if (isModuleGloballyExcluded(&M)) {
    SINFO("Excluding module [{}]", M.getName());
    return PreservedAnalyses::all();
  }

  bool Changed = false;
  SINFO("[{}] Executing on module {}", name(), M.getName());

  for (Function &F : M) {
    if (isFunctionGloballyExcluded(&F))
      continue;

    std::string Name  = demangle(F.getName().str());
    StringRef NRef = Name;
    if (NRef.startswith("_JNIEnv::") && Config.InlineJniWrappers) {
      SDEBUG("[{}] Inlining {}", Name);
      F.addFnAttr(Attribute::AlwaysInline);
      Changed = true;
    }
  }

  if (Config.ShuffleFunctions) {
    shuffleFunctions(M);
    Changed = true;
  }

  SINFO("[{}] Changes {} applied on module {}", name(), Changed ? "" : "not",
        M.getName());

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // end namespace omvll
