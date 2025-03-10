//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "llvm/IR/Constants.h"
#include "llvm/Support/Regex.h"

#include "omvll/ObfuscationConfig.hpp"
#include "omvll/PyConfig.hpp"
#include "omvll/log.hpp"
#include "omvll/passes/objc-cleaner/ObjCleaner.hpp"
#include "omvll/utils.hpp"

using namespace llvm;

namespace omvll {

inline bool isObjCVar(const GlobalVariable &G) {
  if (!G.hasName())
    return false;

  StringRef Name = G.getName();
  return Name.startswith("OBJC_") || Name.startswith("_OBJC_");
}

PreservedAnalyses ObjCleaner::run(Module &M, ModuleAnalysisManager &FAM) {
  bool Changed = false;
  if (isModuleExcluded(&M)) {
    SINFO("Excluding module [{}]", M.getName());
    return PreservedAnalyses::all();
  }

  SINFO("[{}] Executing on module {}", name(), M.getName());

  for (GlobalVariable &G : M.globals()) {
    if (!isObjCVar(G))
      continue;

    if (!G.hasInitializer())
      continue;

    SDEBUG("[{}] ObjC -> {} {}", name(), G.getName().str(),
           ToString(*G.getValueType()));

    if (auto *CSTy = dyn_cast<ConstantStruct>(G.getInitializer())) {
      if (CSTy->getType()->getName().contains("_objc_method")) {
        // TODO: Should these be handled?
        ;
      }
    }

    if (G.isNullValue() || G.isZeroValue())
      continue;

    if (!(G.isConstant() && G.hasInitializer()))
      continue;

    ConstantDataSequential *Data =
        dyn_cast<ConstantDataSequential>(G.getInitializer());

    if (!Data || !Data->isCString())
      continue;

    SDEBUG("[{}] ObjC Var: {}: {}", name(), G.getName(),
           Data->getAsCString().str());

    std::string Str = Data->getAsCString().str();
    SINFO("String: {}", Str);
    Regex R("SampleClass");
    std::string Rep = R.sub("NewName", Str);
    Constant *NewVal = ConstantDataArray::getString(M.getContext(), Rep);

    G.setInitializer(NewVal);
  }

  SINFO("[{}] Changes {} applied on module {}", name(), Changed ? "" : "not",
        M.getName());

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // end namespace omvll
