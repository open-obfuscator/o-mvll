#include "omvll/passes/objcleaner/ObjCleaner.hpp"
#include "omvll/log.hpp"
#include "omvll/utils.hpp"
#include "omvll/PyConfig.hpp"
#include "omvll/passes/Metadata.hpp"
#include "omvll/ObfuscationConfig.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/Support/Regex.h>

using namespace llvm;

namespace omvll {

inline bool isObjCVar(const GlobalVariable& G) {
  if (!G.hasName()) {
    return false;
  }

  StringRef GName = G.getName();
  return GName.startswith("OBJC_") || GName.startswith("_OBJC_");
}

PreservedAnalyses ObjCleaner::run(Module &M,
                                  ModuleAnalysisManager &FAM) {
  SINFO("[{}] Executing on module {}", name(), M.getName());
  bool Changed = false;

  for (GlobalVariable& G : M.getGlobalList()) {
    if (!isObjCVar(G)) {
      continue;
    }

    if (!G.hasInitializer()) {
      continue;
    }

    SDEBUG("[{}] ObjC -> {} {}", name(), G.getName().str(),
           ToString(*G.getValueType()));

    if (auto *CSTy = dyn_cast<ConstantStruct>(G.getInitializer())) {
      if (CSTy->getType()->getName().contains("_objc_method")) {
      }
    }

    if (G.isNullValue() || G.isZeroValue()) {
      continue;
    }

    if (!(G.isConstant() && G.hasInitializer())) {
      continue;
    }

    ConstantDataSequential* data = dyn_cast<ConstantDataSequential>(G.getInitializer());

    if (data == nullptr || !data->isCString()) {
      continue;
    }
    SDEBUG("[{}] ObjC Var: {}: {}", name(), G.getName(),
           data->getAsCString().str());
    //std::string value = data->getAsCString().str();
    //SINFO("String: {}", value);
    //Regex R("SampleClass");
    //std::string rep = R.sub("NewName", value);
    //Constant* NewVal = ConstantDataArray::getString(M.getContext(), rep);
    //G.setInitializer(NewVal);
  }

  //for (Function& F : M) {
  //  Changed |= runOnFunction(F);
  //}

  SINFO("[{}] Changes{}applied on module {}", name(), Changed ? " " : " not ",
        M.getName());

  return Changed ? PreservedAnalyses::none() :
                   PreservedAnalyses::all();

}
}

