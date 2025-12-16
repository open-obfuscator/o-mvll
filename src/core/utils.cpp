//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include <optional>
#include <unistd.h>

#include "llvm/ADT/Hashing.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/RandomNumberGenerator.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/Local.h"

#include "omvll/ObfuscationConfig.hpp"
#include "omvll/PyConfig.hpp"
#include "omvll/log.hpp"
#include "omvll/omvll_config.hpp"
#include "omvll/utils.hpp"

using namespace llvm;

namespace detail {
static std::mutex ModuleCompilation;

static int runExecutable(SmallVectorImpl<StringRef> &Args,
                         std::optional<ArrayRef<StringRef>> Envs = std::nullopt,
                         ArrayRef<std::optional<StringRef>> Redirects = {}) {
  std::string Buffer;
  raw_string_ostream CmdStr(Buffer);
  for (StringRef Arg : Args)
    CmdStr << Arg << " ";
  SINFO("Invoke subprocess: {}", CmdStr.str());
  return sys::ExecuteAndWait(Args[0], Args, Envs, Redirects);
}

static Expected<std::string> getAppleClangPath() {
  static int Unused;
  std::string HostExePath =
      sys::fs::getMainExecutable("clang", (void *)&Unused);

  // clang, clang++, clang-17, etc. are fine
  if (sys::path::filename(HostExePath).starts_with("clang"))
    return HostExePath;

  SmallString<128> ClangPath = sys::path::parent_path(HostExePath);
  llvm::sys::path::append(ClangPath, "clang");
  if (llvm::sys::fs::exists(ClangPath))
    return std::string(ClangPath);

  return createStringError(inconvertibleErrorCode(),
                           "Cannot find clang compiler: " + ClangPath);
}

static Expected<std::string> getIPhoneOSSDKPath() {
  ErrorOr<std::string> MaybeXcrunPath = sys::findProgramByName("xcrun");
  if (!MaybeXcrunPath)
    return createStringError(MaybeXcrunPath.getError(),
                             "Unable to find xcrun.");
  auto XcrunPath = *MaybeXcrunPath;

  int FD;
  SmallString<128> TempPath;
  if (std::error_code EC =
          sys::fs::createTemporaryFile("xcrun-output", "txt", FD, TempPath))
    return createStringError(EC, "Failed to create temporary file.");

  SmallVector<StringRef, 8> Args = {XcrunPath, "--sdk", "iphoneos",
                                    "--show-sdk-path"};

  Expected<std::string> ClangPath = getAppleClangPath();
  if (!ClangPath)
    return ClangPath.takeError();
  size_t DirPos = ClangPath->find("/Contents/Developer");
  std::string DeveloperDir =
      ClangPath->substr(0, DirPos + strlen("/Contents/Developer"));
  const auto &DeveloperDirEnvVar = "DEVELOPER_DIR=" + DeveloperDir;
  SmallVector<StringRef, 1> Envs = {DeveloperDirEnvVar};

  std::optional<StringRef> Redirects[] = {std::nullopt, StringRef(TempPath),
                                          std::nullopt};

  if (int EC = runExecutable(Args, Envs, Redirects))
    return createStringError(inconvertibleErrorCode(),
                             "Unable to execute program: " +
                                 std::to_string(EC));

  std::string Out;
  auto Buffer = MemoryBuffer::getFile(TempPath);
  if (Buffer && *Buffer)
    Out = Buffer->get()->getBuffer().rtrim();
  else
    return createStringError(Buffer.getError(), "Unable to read output.");

  (void)sys::fs::remove(TempPath);
  return Out;
}

static Expected<std::string>
runClangExecutable(StringRef Code, StringRef Dashx, const Triple &Triple,
                   const std::vector<std::string> &ExtraArgs) {
  Expected<std::string> ClangPath = getAppleClangPath();
  if (!ClangPath)
    return ClangPath.takeError();

  SmallVector<StringRef, 16> Args = {*ClangPath, "-S", "-emit-llvm"};

  // Always add the target triple.
  const auto &TargetTriple = Triple.str();
  Args.push_back("-target");
  Args.push_back(TargetTriple);

  std::string SDKPath;
  if (Triple.isiOS()) {
    auto MaybeSDKPath = getIPhoneOSSDKPath();
    if (Error E = MaybeSDKPath.takeError()) {
      SWARN("Warning: {}", toString(std::move(E)));
    } else {
      SDKPath = *MaybeSDKPath;
      Args.push_back("-isysroot");
      Args.push_back(SDKPath);
    }
  }

  for (const auto &Arg : ExtraArgs)
    Args.push_back(Arg);

  // Create the input C file and choose macthing output file name.
  SmallString<256> WorkDir;
  if (!omvll::Config.OutputFolder.empty()) {
    WorkDir = omvll::Config.OutputFolder;
  } else {
    sys::fs::current_path(WorkDir);
    sys::path::append(WorkDir, "omvll-tmp");
  }
  sys::path::append(WorkDir, "cache");
  sys::fs::create_directories(WorkDir);

  // Create input file.
  int InFileFD = -1;
  SmallString<256> InFileName;
  std::string Prefix = "omvll-cache-" + Triple.getTriple();

  // Generate Template with the expected name.
  SmallString<256> Template(WorkDir);
  sys::path::append(Template, Prefix + "-%%%%%%." + Dashx.str());

  if (std::error_code EC =
          sys::fs::createUniqueFile(Template, InFileFD, InFileName))
    return errorCodeToError(EC);

  // Output file next to it.
  SmallString<256> OutFileName(InFileName);
  sys::path::replace_extension(OutFileName, "ll");

  Args.push_back("-o");
  Args.push_back(OutFileName);
  Args.push_back(InFileName);

  // Write the given C code to the input file.
  {
    raw_fd_ostream OS(InFileFD, /*shouldClose=*/true);
    OS << Code;
  }

  if (int EC = runExecutable(Args))
    return createStringError(inconvertibleErrorCode(),
                             Twine("exit code ") + std::to_string(EC));

  return OutFileName.str().str();
}

static Error createSMDiagnosticError(SMDiagnostic &Diag) {
  std::string Msg;
  {
    raw_string_ostream OS(Msg);
    Diag.print("", OS);
  }
  return make_error<StringError>(std::move(Msg), inconvertibleErrorCode());
}

static Expected<std::unique_ptr<Module>> loadModule(StringRef Path,
                                                    LLVMContext &Ctx) {
  SMDiagnostic Err;
  auto M = parseIRFile(Path, Err, Ctx);
  if (!M)
    return createSMDiagnosticError(Err);
  return M;
}

} // end namespace detail

namespace omvll {

unsigned getPid() { return ::getpid(); }

std::string ToString(const Module &M) {
  std::error_code EC;
  std::string Out;
  raw_string_ostream OS(Out);
  M.print(OS, nullptr);
  return Out;
}

std::string ToString(const Instruction &I) {
  std::string Out;
  raw_string_ostream(Out) << I;
  return Out;
}

std::string ToString(const BasicBlock &BB) {
  std::string Out;
  raw_string_ostream OS(Out);
  BB.printAsOperand(OS, true);
  return Out;
}

std::string ToString(const Type &Ty) {
  std::string Out;
  raw_string_ostream OS(Out);
  OS << TypeIDStr(Ty) << ": " << Ty;
  return Out;
}

std::string ToString(const Value &V) {
  std::string Out;
  raw_string_ostream OS(Out);
  OS << ValueIDStr(V) << ": " << V;
  return Out;
}

std::string ToString(const MDNode &N) {
  std::string Out;
  raw_string_ostream OS(Out);
  N.printTree(OS);
  return Out;
}

std::string TypeIDStr(const Type &Ty) {
  switch (Ty.getTypeID()) {
    case Type::TypeID::HalfTyID:           return "HalfTyID";
    case Type::TypeID::BFloatTyID:         return "BFloatTyID";
    case Type::TypeID::FloatTyID:          return "FloatTyID";
    case Type::TypeID::DoubleTyID:         return "DoubleTyID";
    case Type::TypeID::X86_FP80TyID:       return "X86_FP80TyID";
    case Type::TypeID::FP128TyID:          return "FP128TyID";
    case Type::TypeID::PPC_FP128TyID:      return "PPC_FP128TyID";
    case Type::TypeID::VoidTyID:           return "VoidTyID";
    case Type::TypeID::LabelTyID:          return "LabelTyID";
    case Type::TypeID::MetadataTyID:       return "MetadataTyID";
    case Type::TypeID::X86_MMXTyID:        return "X86_MMXTyID";
    case Type::TypeID::X86_AMXTyID:        return "X86_AMXTyID";
    case Type::TypeID::TokenTyID:          return "TokenTyID";
    case Type::TypeID::IntegerTyID:        return "IntegerTyID";
    case Type::TypeID::FunctionTyID:       return "FunctionTyID";
    case Type::TypeID::PointerTyID:        return "PointerTyID";
    case Type::TypeID::StructTyID:         return "StructTyID";
    case Type::TypeID::ArrayTyID:          return "ArrayTyID";
    case Type::TypeID::FixedVectorTyID:    return "FixedVectorTyID";
    case Type::TypeID::ScalableVectorTyID: return "ScalableVectorTyID";
    case Type::TypeID::TypedPointerTyID:
      return "TypedPointerTyID";
    default:
      llvm_unreachable("Unhandled TypeID!");
  }
}

std::string ValueIDStr(const Value &V) {

#define HANDLE_VALUE(ValueName) case Value::ValueTy::ValueName ## Val: return #ValueName;
//#define HANDLE_INSTRUCTION(Name)  /* nothing */
  switch (V.getValueID()) {
    #include "llvm/IR/Value.def"
  }

#define HANDLE_INST(N, OPC, CLASS) case N: return #CLASS;
  switch (V.getValueID() - Value::ValueTy::InstructionVal) {
    #include "llvm/IR/Instruction.def"
  }
  return std::to_string(V.getValueID());
}

void dump(Module &M, const std::string &File) {
  std::error_code EC;
  raw_fd_ostream FD(File, EC);
  M.print(FD, nullptr);
}

void dump(Function &F, const std::string &File) {
  std::error_code EC;
  raw_fd_ostream FD(File, EC);
  F.print(FD, nullptr);
}

void dump(const MemoryBuffer &MB, const std::string &File) {
  std::error_code EC;
  raw_fd_ostream FD(File, EC);
  FD << MB.getBuffer();
}

size_t demotePHINode(Function &F) {
  size_t Count = 0;
  std::vector<PHINode *> PhiNodes;
  do {
    PhiNodes.clear();
    for (auto &BB : F)
      for (auto &I : BB.phis())
        PhiNodes.push_back(&I);

    Count += PhiNodes.size();
    for (PHINode *Phi : PhiNodes)
#if LLVM_VERSION_MAJOR > 18
      DemotePHIToStack(Phi, F.begin()->getTerminator()->getIterator());
#else
      DemotePHIToStack(Phi, F.begin()->getTerminator());
#endif
    } while (!PhiNodes.empty());
  return Count;
}

static bool valueEscapes(const Instruction &Inst) {
  const BasicBlock *BB = Inst.getParent();
  for (const User *U : Inst.users()) {
    const Instruction *UI = cast<Instruction>(U);
    if (UI->getParent() != BB || isa<PHINode>(UI))
      return true;
  }
  return false;
}

size_t demoteRegs(Function &F) {
  size_t Count = 0;
  std::list<Instruction *> WorkList;
  BasicBlock *BBEntry = &F.getEntryBlock();
  do {
    WorkList.clear();
    for (BasicBlock &BB : F)
      for (Instruction &I : BB) {
        if (&BB == BBEntry || isa<AllocaInst>(I))
          continue;
        if (valueEscapes(I))
          WorkList.push_front(&I);
      }

    Count += WorkList.size();
    for (Instruction *I : WorkList)
#if LLVM_VERSION_MAJOR > 18
      DemoteRegToStack(*I, false, F.begin()->getTerminator()->getIterator());
#else
      DemoteRegToStack(*I, false, F.begin()->getTerminator());
#endif
  } while (!WorkList.empty());
  return Count;
}

size_t reg2mem(Function &F) {
  /* The code of this function comes from the pass Reg2Mem.cpp
   * Note(Romain): I tried to run this pass using the PassManager with the following code:
   *
   *    FunctionAnalysisManager FAM;
   *    FAM.registerPass([&] { return DominatorTreeAnalysis(); });
   *    FAM.registerPass([&] { return LoopAnalysis(); });
   *
   *    FunctionPassManager FPM;
   *    FPM.addPass(Reg2MemPass())
   *
   *    FPM.run(F, FAM);
   * but it fails on Xcode (crash in AnalysisManager::getResultImp).
   * It might be worth investigating why it crashes.
   */
  SplitAllCriticalEdges(F, CriticalEdgeSplittingOptions());
  size_t Count = 0;
  // Insert all new allocas into entry block.
  BasicBlock *BBEntry = &F.getEntryBlock();
  assert(pred_empty(BBEntry) &&
         "Entry block to function must not have predecessors!");

  // Find first non-alloca instruction and create insertion point. This is
  // safe if block is well-formed: it always have terminator, otherwise
  // we'll get and assertion.
  BasicBlock::iterator I = BBEntry->begin();
  while (isa<AllocaInst>(I)) ++I;

  CastInst *BCI = new BitCastInst(
      Constant::getNullValue(Type::getInt32Ty(F.getContext())),
      Type::getInt32Ty(F.getContext()), "reg2mem alloca point", &*I);

#if LLVM_VERSION_MAJOR > 18
  auto AllocaInsertionPoint = BCI->getIterator();
#else
  auto AllocaInsertionPoint = BCI;
#endif  

  // Find the escaped instructions. But don't create stack slots for
  // allocas in entry block.
  std::list<Instruction *> WorkList;
  for (Instruction &I : instructions(F))
    if (!(isa<AllocaInst>(I) && I.getParent() == BBEntry) && valueEscapes(I))
      WorkList.push_front(&I);

  // Demote escaped instructions.
  Count += WorkList.size();
  for (Instruction *I : WorkList)
    DemoteRegToStack(*I, false, AllocaInsertionPoint);

  WorkList.clear();

  // Find all phi's.
  for (BasicBlock &BB : F)
    for (PHINode &Phi : BB.phis())
      WorkList.push_front(&Phi);

  // Demote phi nodes.
  Count += WorkList.size();
  for (Instruction *I : WorkList)
    DemotePHIToStack(cast<PHINode>(I), AllocaInsertionPoint);

  return Count;
}

void shuffleFunctions(Module &M) {
  /*
   * The iterator associated getFunctionList() is not "random"
   * so we can't std::shuffle() the list.
   *
   * On the other hand, getFunctionList() has a sort method that we can
   * use to (randomly?) shuffle in list in place.
   */
  DenseMap<Function *, uint32_t> Values;
  DenseSet<uint64_t> Taken;

  std::mt19937_64 Gen64;
  size_t Max = std::numeric_limits<uint32_t>::max();
  std::uniform_int_distribution<uint32_t> Dist(0, Max - 1);

  for (Function &F : M) {
    uint64_t ID = Dist(Gen64);
    while (!Taken.insert(ID).second)
      ID = Dist(Gen64);

    Values[&F] = ID;
  }

  auto &List = M.getFunctionList();
  List.sort([&Values](Function &LHS, Function &RHS) {
    return Values[&LHS] < Values[&RHS];
  });
}

void fatalError(std::string_view Msg) {
  static LLVMContext Ctx;
  SERR("Error: {}", Msg);
  Ctx.emitError(Msg);

  // emitError could return, so we make sure that we stop the execution.
  std::abort();
}

Expected<std::unique_ptr<Module>>
generateModule(StringRef Routine, const Triple &Triple, StringRef Extension,
               LLVMContext &Ctx, ArrayRef<std::string> ExtraArgs) {
  using namespace ::detail;

  std::lock_guard<std::mutex> Lock(ModuleCompilation);
  hash_code HashValue = hash_combine(Routine, Triple.getTriple());

  // TempPath depends on Output folder availability. If it is not present
  // assign current path to avoid problems on parallel builds
  SmallString<256> TempPath;
  if (!Config.OutputFolder.empty()) {
    TempPath = Config.OutputFolder;
  } else {
    sys::fs::current_path(TempPath);
    sys::path::append(TempPath, "omvll-tmp");
  }

  sys::path::append(TempPath, "cache");
  llvm::sys::fs::create_directories(TempPath);
  sys::path::append(TempPath, "omvll-cache-" + Triple.getTriple() + "-" +
                                  std::to_string(HashValue) + "-" +
                                  std::to_string(getPid()) + ".ll");
  std::string IRModuleFilename = std::string(TempPath.str());

  if (sys::fs::exists(IRModuleFilename)) {
    return loadModule(IRModuleFilename, Ctx);
  } else {
    auto MaybePath = runClangExecutable(Routine, Extension, Triple, ExtraArgs);
    if (!MaybePath)
      return MaybePath.takeError();

    auto MaybeModule = loadModule(*MaybePath, Ctx);
    if (!MaybeModule)
      return MaybeModule.takeError();

    std::error_code EC;
    raw_fd_ostream IRModuleFile(IRModuleFilename, EC, sys::fs::OF_None);
    if (EC)
      return errorCodeToError(EC);

    (*MaybeModule)->print(IRModuleFile, nullptr);

    return MaybeModule;
  }
}

IRChangesMonitor::IRChangesMonitor(const Module &M, StringRef PassName)
    : M(M), UserConfig(PyConfig::instance().getUserConfig()),
      PassName(PassName), ChangeReported(false) {
  if (UserConfig->hasReportDiffOverride())
    raw_string_ostream(OriginalIR) << M;
}

PreservedAnalyses IRChangesMonitor::report() {
  if (ChangeReported && UserConfig->hasReportDiffOverride()) {
    std::string ObfuscatedIR;
    raw_string_ostream(ObfuscatedIR) << M;
    if (OriginalIR != ObfuscatedIR)
      UserConfig->reportDiff(PassName, OriginalIR, ObfuscatedIR);
  }

  return ChangeReported ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

bool isModuleGloballyExcluded(Module *M) {
  auto Begin = Config.GlobalModuleExclude.begin();
  auto End = Config.GlobalModuleExclude.end();
  auto It = std::find_if(Begin, End, [&](const auto &ExcludedModule) {
    return M->getName().contains(ExcludedModule);
  });
  return It != End;
}

bool isFunctionGloballyExcluded(Function *F) {
  return is_contained(Config.GlobalFunctionExclude, F->getName());
}

bool isCoroutine(Function *F) {
  for (const Instruction &I : instructions(F)) {
    if (const auto *II = dyn_cast<IntrinsicInst>(&I)) {
      switch (II->getIntrinsicID()) {
      case Intrinsic::coro_begin:
      case Intrinsic::coro_id:
      case Intrinsic::coro_suspend:
      case Intrinsic::coro_suspend_retcon:
      case Intrinsic::coro_resume:
      case Intrinsic::coro_destroy:
      case Intrinsic::coro_end:
      case Intrinsic::coro_frame:
      case Intrinsic::coro_save:
      case Intrinsic::coro_alloc:
      case Intrinsic::coro_free:
        return true;
      default:
        break;
      }
    }
  }

  return false;
}

bool containsSwiftErrorAlloca(const BasicBlock &BB) {
  for (const Instruction &I : BB)
    if (const auto *AI = dyn_cast<AllocaInst>(&I))
      if (AI->isSwiftError())
        return true;
  return false;
}

bool isEHBlock(const BasicBlock &BB) {
  return BB.isEHPad() || BB.isLandingPad();
}

// Default value is false.
bool RandomGenerator::Seeded = false;

int RandomGenerator::generate() {
  if (!RandomGenerator::Seeded) {
    std::srand(Config.ProbabilitySeed);
    RandomGenerator::Seeded = true;
  }

  // Generate number between 0 and 99.
  return std::rand() % 100;
}

int RandomGenerator::checkProbability(int Target) {
  if (RandomGenerator::generate() < Target)
    return true;
  return false;
}

void inlineWithoutLifetimeMarkers(CallInst *Call) {
  InlineFunctionInfo IFI;
  InlineResult Result = InlineFunction(*Call, IFI,
                                       /*MergeAttributes=*/false,
                                       /*CalleeAAR=*/nullptr,
                                       /*InsertLifetime=*/false);
  if (!Result.isSuccess())
    fatalError(Result.getFailureReason());
}

} // end namespace omvll
