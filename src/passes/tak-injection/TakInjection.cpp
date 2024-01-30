#include "omvll/passes/tak-injection/TakInjection.hpp"
#include "omvll/Jitter.hpp"
#include "omvll/ObfuscationConfig.hpp"
#include "omvll/PyConfig.hpp"
#include "omvll/log.hpp"
#include "omvll/passes/tak-injection/PostCoding.hpp"
#include "omvll/utils.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/Host.h>
#include <llvm/Transforms/Utils/Cloning.h>

using namespace llvm;

static constexpr std::array InitializerFunctions = {
    "-[AppDelegate application:didFinishLaunchingWithOptions:]",
    "AppDelegateC11application_"
    "29didFinishLaunchingWithOptionsSbSo13UIApplicationC_"
    "SDySo0m6LaunchL3KeyaypGSgtFTo",
    "AppDelegateC11application_"
    "29didFinishLaunchingWithOptionsSbSo13UIApplicationC_"
    "SDySo0l6LaunchK3KeyaypGSgtFTo"};

static bool isInitializerFunction(const Function &F) {
  for (const auto &IF : InitializerFunctions)
    if (F.getName().endswith(IF))
      return true;
  return false;
}

static bool injectTakRuntimeProtection(Module &M, const StringRef Name) {
  bool Changed = false;

  for (Function &F : M) {
    if (!isInitializerFunction(F))
      continue;

    auto &Ctx = M.getContext();

    // Invoke host clang and lower the __tak_injection Objective-C++ function to
    // an LLVM IR module.
    std::unique_ptr<Module> TakInjectionIRModule;
    ExitOnError ExitOnErr("Clang invocation failed: ");

    {
      // Generate module.
      Ctx.setDiscardValueNames(false);
      TakInjectionIRModule = ExitOnErr(omvll::generateModule(
          omvll::TakInjectionFunction, Triple(M.getTargetTriple()), "mm", Ctx,
          {"-fblocks"}));
    }

    // Verify the newly-created module is not broken.
    if (llvm::verifyModule(*TakInjectionIRModule, &llvm::dbgs()))
      report_fatal_error("Broken module found, compilation aborted.");

    // Link the __tak_injection module into the composite.
    if (!M.getFunction(TakInjectionFunctionName)) {
      llvm::Linker TheLinker(M);
      bool Result =
          TheLinker.linkInModule(std::move(TakInjectionIRModule), Linker::None);
      if (Result)
        report_fatal_error("Linkage failed, compilation aborted.");
    }

    // Set function linkage to external.
    auto *FT = FunctionType::get(Type::getVoidTy(Ctx), {}, false);
    FunctionCallee TakInjectionCallee =
        M.getOrInsertFunction(TakInjectionFunctionName, FT);
    if (Function *TakFn = dyn_cast<Function>(TakInjectionCallee.getCallee()))
      TakFn->setLinkage(GlobalValue::ExternalLinkage);

    // Inject a function call to __tak_injection at the entry block of the
    // initializer function found.
    auto &Entry = F.getEntryBlock();
    IRBuilder<> IRB(&Entry);
    IRB.SetInsertPoint(Entry.getFirstNonPHIOrDbg(true));
    CallInst *CI = IRB.CreateCall(TakInjectionCallee);

    // Attempt to inline the __tak_injection call site.
    InlineFunctionInfo IFI;
    if (!InlineFunction(*CI, IFI).isSuccess())
      SDEBUG("[{}] Could not inline call-site.", Name);

    Changed = true;
    break;
  }

  return Changed;
}

namespace omvll {

PreservedAnalyses TakInjection::run(Module &M, ModuleAnalysisManager &FAM) {
  bool Changed = false;

  Changed |= injectTakRuntimeProtection(M, name());
  if (Changed)
    SINFO("[{}] Done!", name());

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

} // namespace omvll
