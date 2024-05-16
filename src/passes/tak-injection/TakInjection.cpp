#include "omvll/passes/tak-injection/TakInjection.hpp"
#include "omvll/Jitter.hpp"
#include "omvll/ObfuscationConfig.hpp"
#include "omvll/PyConfig.hpp"
#include "omvll/log.hpp"
#include "omvll/passes/tak-injection/PostCoding.hpp"
#include "omvll/utils.hpp"

#include "llvm/Support/Path.h"
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/Cloning.h>

#include <regex>
#include <string>

using namespace llvm;

static bool isInitializerFunction(const Function &F) {
  const std::string &Name = F.getName().str();

  // So far, we found the following methods signature:
  // "\U00000001-[AppDelegate application:didFinishLaunchingWithOptions:]"
  // "$s1511AppDelegateC11application_29didFinishLaunchingWithOptionsSbSo13UIApplicationC_SDySo0m6LaunchL3KeyaypGSgtFTo"
  // "$s1511AppDelegateC11application_29didFinishLaunchingWithOptionsSbSo13UIApplicationC_SDySo0l6LaunchK3KeyaypGSgtFTo"
  //
  // The regex is a or between the ObjC name and the Swift-mangled ones, with
  // some care for the Unicode char (ObjC) and mangled parameters (Swift).
  // Note that all Swift-mangled names begin with a common prefix, ref at:
  // https://github.com/apple/swift/blob/main/docs/ABI/Mangling.rst.

  std::regex Expr(
      "^(.*-\\[AppDelegate\\sapplication:"
      "didFinishLaunchingWithOptions:.*|\\$s.*AppDelegate.*application.*"
      "didFinishLaunchingWithOptions.*UIApplication.*)");

  return std::regex_match(Name, Expr);
}

static bool recordInjectionOrFail() {
  SmallString<256> Path;
  sys::path::system_temp_directory(true, Path);
  SDEBUG("Temporary folder: {}", Path.str());
  sys::path::append(Path, "omvll_tak_injection");

  int ResultFD;
  std::error_code EC = sys::fs::openFileForReadWrite(
      Path, ResultFD, sys::fs::CD_CreateNew, sys::fs::OF_None);
  if (EC) {
    if (EC == std::errc::file_exists)
      SDEBUG("File already exists.");
    return false;
  }
  return true;
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

    if (!recordInjectionOrFail())
      report_fatal_error("Ensuring TAK was injected only once failed, please "
                         "contact Build38.");

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
