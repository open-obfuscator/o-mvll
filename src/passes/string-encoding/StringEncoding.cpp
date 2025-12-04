//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include <string>

#include "llvm/ADT/STLExtras.h"
#if LLVM_VERSION_MAJOR > 18
#include "llvm/ADT/StableHashing.h"
#else
#include "llvm/CodeGen/StableHashing.h"
#endif
#include "llvm/ADT/StringExtras.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/NoFolder.h"
#include "llvm/Support/xxhash.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"

#include "omvll/ObfuscationConfig.hpp"
#include "omvll/PyConfig.hpp"
#include "omvll/jitter.hpp"
#include "omvll/log.hpp"
#include "omvll/passes/Metadata.hpp"
#include "omvll/passes/arithmetic/Arithmetic.hpp"
#include "omvll/passes/string-encoding/StringEncoding.hpp"
#include "omvll/utils.hpp"
#include "omvll/visitvariant.hpp"

using namespace llvm;

static ExitOnError ExitOnErr;

static constexpr auto DecodeFunctionName = "__omvll_decode";
static constexpr auto CtorPrefixName = "__omvll_ctor_";

namespace omvll {

inline bool isEligible(const GlobalVariable &G) {
  if (G.isNullValue() || G.isZeroValue())
    return false;

  if (!(G.isConstant() && G.hasInitializer()))
    return false;

  return true;
}

inline bool isEligible(const ConstantDataSequential &CDS) {
  return CDS.isCString() && CDS.getAsCString().size() > 1;
}

inline bool isSkip(const StringEncodingOpt &EncInfo) {
  return std::get_if<StringEncOptSkip>(&EncInfo) != nullptr;
}

GlobalVariable *extractGlobalVariable(ConstantExpr *Expr) {
  while (Expr) {
    if (Expr->getOpcode() == Instruction::IntToPtr ||
        Instruction::isBinaryOp(Expr->getOpcode())) {
      Expr = dyn_cast<ConstantExpr>(Expr->getOperand(0));
    } else if (Expr->getOpcode() == Instruction::PtrToInt ||
               Expr->getOpcode() == Instruction::GetElementPtr) {
      return dyn_cast<GlobalVariable>(Expr->getOperand(0));
    } else {
      break;
    }
  }

  return nullptr;
}

std::pair<Instruction *, Instruction *>
materializeConstantExpression(Instruction *Point, ConstantExpr *CE) {
  auto *Inst = CE->getAsInstruction();
  auto *Prev = Inst;
  Inst->insertBefore(Point);

  Value *Expr = Inst->getOperand(0);
  while (isa<ConstantExpr>(Expr)) {
    auto *NewInst = cast<ConstantExpr>(Expr)->getAsInstruction();
    NewInst->insertBefore(Prev);
    Prev->setOperand(0, NewInst);
    Expr = NewInst->getOperand(0);
    Prev = NewInst;
  }

  return {Inst, Prev};
}

static CallInst *
createDecodingTrampoline(GlobalVariable &G, Use &EncPtr, Instruction *NewPt,
                         uint64_t KeyValI64, uint64_t Size,
                         const StringEncoding::EncodingInfo &EI,
                         bool IsLocalToFunction = false) {
  // Allocas first.
  auto It = NewPt->getFunction()->getEntryBlock().begin();
  while (It->getOpcode() == Instruction::Alloca)
    ++It;

  IRBuilder<NoFolder> IRB(&*It);
  auto *BufferTy = ArrayType::get(IRB.getInt8Ty(), Size);
  auto *M = NewPt->getModule();
  GlobalVariable *ClearBuffer =
      new GlobalVariable(*M, BufferTy, false, GlobalValue::InternalLinkage,
                         Constant::getNullValue(BufferTy));

  AllocaInst *Key = IRB.CreateAlloca(IRB.getInt64Ty());
  AllocaInst *StrSize = IRB.CreateAlloca(IRB.getInt32Ty());

  IRB.SetInsertPoint(NewPt);
  auto *StoreKey = IRB.CreateStore(IRB.getInt64(KeyValI64), Key);
  addMetadata(*StoreKey, MetaObf(OpaqueCst));

  IRB.CreateStore(IRB.getInt32(Size), StrSize);

  LoadInst *LoadKey = IRB.CreateLoad(IRB.getInt64Ty(), Key);
  addMetadata(*LoadKey, MetaObf(OpaqueCst));

  auto *KeyVal = IRB.CreateBitCast(LoadKey, IRB.getInt64Ty());

  LoadInst *LStrSize = IRB.CreateLoad(IRB.getInt32Ty(), StrSize);
  addMetadata(*LStrSize, MetaObf(OpaqueCst));

  auto *VStrSize = IRB.CreateBitCast(LStrSize, IRB.getInt32Ty());

  Function *FDecode = EI.TM->getFunction("decode");
  if (!FDecode)
    fatalError("Cannot find decode routine");

  // TODO: support ObjC strings as well
  if (isa<ConstantExpr>(EncPtr))
    assert(extractGlobalVariable(cast<ConstantExpr>(EncPtr)) == &G &&
           "Previously extracted global variable need to match");

  Value *Input = IRB.CreateBitCast(&G, IRB.getPtrTy());
  Value *Output = Input;

  if (IsLocalToFunction)
    Output = IRB.CreateInBoundsGEP(BufferTy, ClearBuffer,
                                   {IRB.getInt64(0), IRB.getInt64(0)});

  auto *NewF =
      Function::Create(FDecode->getFunctionType(), GlobalValue::PrivateLinkage,
                       DecodeFunctionName, NewPt->getModule());

  ValueToValueMapTy VMap;
  auto NewFArgsIt = NewF->arg_begin();
  auto FArgsIt = FDecode->arg_begin();
  for (auto FArgsEnd = FDecode->arg_end(); FArgsIt != FArgsEnd;
       ++NewFArgsIt, ++FArgsIt)
    VMap[&*FArgsIt] = &*NewFArgsIt;

  SmallVector<ReturnInst *, 8> Returns;
  CloneFunctionInto(NewF, FDecode, VMap,
                    CloneFunctionChangeType::DifferentModule, Returns);
  NewF->setDSOLocal(true);

  std::vector<Value *> Args = {Output, Input, KeyVal, VStrSize};

  if (NewF->arg_size() != 4)
    fatalError(
        fmt::format("Expecting 4 arguments ({} provided)", NewF->arg_size()));

  for (size_t Idx = 0; Idx < NewF->arg_size(); ++Idx) {
    auto *E = NewF->getFunctionType()->getParamType(Idx);
    auto *V = Args[Idx]->getType();
    if (E != V)
      fatalError(fmt::format("Args #{}: Expecting {} while it is {}", Idx,
                             ToString(*E), ToString(*V)));
  }

  if (!IsLocalToFunction)
    return IRB.CreateCall(NewF->getFunctionType(), NewF, Args);

  auto *BoolType = IRB.getInt1Ty();
  GlobalVariable *NeedDecode =
      new GlobalVariable(*M, BoolType, false, GlobalValue::InternalLinkage,
                         ConstantInt::getFalse(BoolType));

  It = IRB.GetInsertPoint();
  auto *WrapperType =
      FunctionType::get(IRB.getVoidTy(),
                        {IRB.getPtrTy(), IRB.getPtrTy(), IRB.getPtrTy(),
                         IRB.getInt64Ty(), IRB.getInt32Ty()},
                        false);
  auto *Wrapper = Function::Create(WrapperType, GlobalValue::PrivateLinkage,
                                   "__omvll_decode_wrap", NewPt->getModule());

  // Decode wrapper to check whether the global variable has already been
  // decoded.
  BasicBlock *Entry = BasicBlock::Create(NewPt->getContext(), "entry", Wrapper);
  IRB.SetInsertPoint(Entry);
  auto *ICmp = IRB.CreateICmpEQ(IRB.CreateLoad(BoolType, Wrapper->getArg(0)),
                                ConstantInt::getFalse(BoolType));
  auto *NewBB = BasicBlock::Create(NewPt->getContext(), "", Wrapper);
  auto *ContinuationBB = BasicBlock::Create(NewPt->getContext(), "", Wrapper);
  IRB.CreateCondBr(ICmp, NewBB, ContinuationBB);
  IRB.SetInsertPoint(NewBB);
  auto *CI = IRB.CreateCall(NewF->getFunctionType(), NewF,
                            {Wrapper->getArg(1), Wrapper->getArg(2),
                             Wrapper->getArg(3), Wrapper->getArg(4)});
  IRB.CreateStore(ConstantInt::getTrue(BoolType), Wrapper->getArg(0));
  IRB.CreateBr(ContinuationBB);
  IRB.SetInsertPoint(ContinuationBB);
  IRB.CreateRetVoid();

  // Insert decode wrapper call site in the caller.
  IRB.SetInsertPoint(NewPt->getParent(), It);
  CI = IRB.CreateCall(Wrapper->getFunctionType(), Wrapper,
                      {NeedDecode, Output, Input, KeyVal, VStrSize});

  if (auto *CE = dyn_cast<ConstantExpr>(EncPtr)) {
    auto [First, Last] = materializeConstantExpression(NewPt, CE);
    assert(((First != Last) ||
            (isa<GetElementPtrInst>(First) || isa<PtrToIntInst>(First))) &&
           "Nested constantexpr in getelementptr/ptrtoint should not appear?");
    if (isa<GetElementPtrInst>(First)) {
      // CE is already a GEP, directly replace the operand with the decode
      // output.
      NewPt->setOperand(EncPtr.getOperandNo(), Output);
      if (isInstructionTriviallyDead(Last))
        Last->eraseFromParent();
    } else {
      Last->setOperand(0, Output);
      NewPt->setOperand(EncPtr.getOperandNo(), First);
    }
  } else {
    NewPt->setOperand(EncPtr.getOperandNo(), Output);
  }

  return CI;
}

bool StringEncoding::encodeStrings(Function &F, ObfuscationConfig &UserConfig) {
  bool Changed = false;
  llvm::Module *M = F.getParent();

  for (Instruction &I : make_early_inc_range(instructions(F))) {
    assert(!isa<PHINode>(I) && "Found phi previously demoted?");

    for (Use &Op : make_early_inc_range(I.operands())) {
      auto *G = dyn_cast<GlobalVariable>(Op->stripPointerCasts());

      // Is the operand a constant expression?
      if (!G)
        if (auto *CE = dyn_cast<ConstantExpr>(Op))
          G = extractGlobalVariable(CE);

      if (!G || !G->hasInitializer())
        continue;

      // Is the global initializer part of a constant expression?
      auto IsInitializerConstantExpr = [](const GlobalVariable &G) {
        return !G.isExternallyInitialized() &&
               isa<ConstantExpr>(G.getInitializer());
      };

      Use *ActualOp = &Op;
      bool MaybeStringInCEInitializer = false;
      if (IsInitializerConstantExpr(*G)) {
        auto *MaybeNestedGV =
            extractGlobalVariable(cast<ConstantExpr>(G->getInitializer()));
        if (MaybeNestedGV && MaybeNestedGV->hasOneUse()) {
          ActualOp = MaybeNestedGV->getSingleUndroppableUse();
          G = MaybeNestedGV;
          MaybeStringInCEInitializer = true;
        }
      }

      // Get the underlying global variable, if pointer casts involved.
      if (auto *StrippedGV = dyn_cast<GlobalVariable>(
              G->getInitializer()->stripPointerCasts());
          StrippedGV)
        G = StrippedGV;

      if (!isEligible(*G))
        continue;

      auto *Data = dyn_cast<ConstantDataSequential>(G->getInitializer());
      if (!Data)
        continue;

      // Create a default option which skips the encoding.
      auto EncInfoOpt = std::make_unique<StringEncodingOpt>(StringEncOptSkip());

      if (EncodingInfo *EI = getEncoding(*G)) {
        // The global variable is already encoded.
        // Let's check if we should insert a decoding stub.
        Changed |= injectDecoding(I, Op, *G, *Data, *EI);
        continue;
      }

      if (isEligible(*Data)) {
        // Get the encoding type from the user.
        EncInfoOpt = std::make_unique<StringEncodingOpt>(
            UserConfig.obfuscateString(M, &F, Data->getAsCString().str()));
      }

      if (isSkip(*EncInfoOpt) ||
          (MaybeStringInCEInitializer &&
           std::get_if<StringEncOptGlobal>(EncInfoOpt.get()) == nullptr))
        continue;

      SINFO("[{}] Processing string {}", name(), Data->getAsCString());
      Changed |= process(I, *ActualOp, *G, *Data, *EncInfoOpt);
    }
  }

  return Changed;
}

PreservedAnalyses StringEncoding::run(Module &M, ModuleAnalysisManager &MAM) {
  if (isModuleGloballyExcluded(&M)) {
    SINFO("Excluding module [{}]", M.getName());
    return PreservedAnalyses::all();
  }

  bool Changed = false;
  SINFO("[{}] Executing on module {}", name(), M.getName());
  auto &Config = PyConfig::instance();
  ObfuscationConfig *UserConfig = Config.getUserConfig();
  RNG = M.createRNG(name());

  std::vector<Function *> ToVisit;
  for (Function &F : M) {
    if (isFunctionGloballyExcluded(&F) || F.empty() || F.isDeclaration())
      continue;

    demotePHINode(F);
    ToVisit.emplace_back(&F);
  }

  for (Function *F : ToVisit) {
    std::string Name = demangle(F->getName().str());
    SDEBUG("[{}] Visiting function {}", name(), Name);
    Changed |= encodeStrings(*F, *UserConfig);
  }

  // Inline functions. Avoid emitting lifetime markers while inlining. After
  // cloning the decode function from a Clang-generated module, the lifetime
  // intrinsic decls created during inlining may end up with incorrect
  // attributes (e.g., swiftself) with Swift modules, causing verification
  // failures.
  for (CallInst *Callee : ToInline)
    inlineWithoutLifetimeMarkers(Callee);

  // Create constructors functions.
  for (Function *F : Ctors)
    appendToGlobalCtors(M, F, 0);

  SINFO("[{}] Changes {} applied on module {}", name(), Changed ? "" : "not",
        M.getName());

  return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

bool StringEncoding::injectDecoding(Instruction &I, Use &Op, GlobalVariable &G,
                                    ConstantDataSequential &Data,
                                    const StringEncoding::EncodingInfo &Info) {
  switch (Info.Type) {
  case EncodingTy::None:
  case EncodingTy::Replace:
  case EncodingTy::Global:
    return false;
  case EncodingTy::Local:
    return injectDecodingLocally(I, Op, G, Data, Info);
  }
  llvm_unreachable("Unhandled case");
}

bool StringEncoding::injectDecodingLocally(
    Instruction &I, Use &Op, GlobalVariable &G, ConstantDataSequential &Data,
    const StringEncoding::EncodingInfo &Info) {
  auto *Key = std::get_if<KeyIntTy>(&Info.Key);
  if (!Key)
    fatalError("String stack decoding loop is expecting a buffer as a key! ");

  uint64_t StrSz = Data.getRawDataValues().size();
  SDEBUG("Key for 0x{:010x}", *Key);

  auto *Callee = createDecodingTrampoline(G, Op, &I, *Key, StrSz, Info, true);
  ToInline.push_back(Callee);
  return true;
}

bool StringEncoding::process(Instruction &I, Use &Op, GlobalVariable &G,
                             ConstantDataSequential &Data,
                             StringEncodingOpt &Opt) {
  bool Changed = std::visit(
      overloaded{
          [&](StringEncOptSkip &) { return false; },
          [&](StringEncOptLocal &) { return processLocal(I, Op, G, Data); },
          [&](StringEncOptGlobal &) { return processGlobal(I, Op, G, Data); },
          [&](StringEncOptReplace &Rep) {
            return processReplace(I, Op, G, Data, Rep);
          },
          [&](StringEncOptDefault &) {
            // Default to local, if no option is specified.
            return processLocal(I, Op, G, Data);
          },
      },
      Opt);
  return Changed;
}

bool StringEncoding::processReplace(Instruction &I, Use &Op, GlobalVariable &G,
                                    ConstantDataSequential &Data,
                                    StringEncOptReplace &Rep) {
  auto &New = Rep.NewString;
  const size_t InLen = New.size();
  SDEBUG("Replacing {} with {}", Data.getAsString().str(), New);

  if (New.size() < Data.getNumElements()) {
    SWARN("'{}' is smaller than the original string. It will be padded with 0",
          New);
    New = New.append(Data.getNumElements() - InLen - 1, '\0');
  } else {
    SWARN("'{}' is larger than the original string. It will be shrinked to fit "
          "the original data",
          New);
    New = New.substr(0, Data.getNumElements() - 1);
  }

  Constant *NewVal = ConstantDataArray::getString(I.getContext(), New);
  G.setInitializer(NewVal);
  GVarEncInfo.insert({&G, EncodingInfo(EncodingTy::Replace)});
  return true;
}

bool StringEncoding::processGlobal(Instruction &I, Use &Op, GlobalVariable &G,
                                   ConstantDataSequential &Data) {
  Module *M = I.getModule();
  LLVMContext &Ctx = I.getContext();
  StringRef Str = Data.getRawDataValues();
  uint64_t StrSz = Str.size();
  size_t Max = std::numeric_limits<KeyIntTy>::max();
  std::uniform_int_distribution<KeyIntTy> Dist(1, Max);
  uint64_t Key = Dist(*RNG);

  SDEBUG("Key for {}: 0x{:010x}", Str.str(), Key);

  std::vector<char> Encoded(StrSz);
  EncodingInfo EI(EncodingTy::Global);
  EI.Key = Key;
  genRoutines(Triple(M->getTargetTriple()), EI, Ctx);

  EI.EncodeFn(Encoded.data(), Str.data(), Key, StrSz);

  Constant *StrEnc = ConstantDataArray::get(Ctx, Encoded);
  G.setConstant(false);
  G.setInitializer(StrEnc);

  // Now create a module constructor for the encoded string.
  FunctionType *FTy = FunctionType::get(Type::getVoidTy(Ctx), /* no args */ {},
                                        /* no var args */ false);

#if LLVM_VERSION_MAJOR > 18
  unsigned GlobalIDHashVal = xxh3_64bits(G.getGlobalIdentifier());
#else
  unsigned GlobalIDHashVal = stable_hash_combine_string(G.getGlobalIdentifier());
#endif
  unsigned HashCombinedVal = stable_hash_combine(GlobalIDHashVal, StrSz, Key);
  std::string Name = CtorPrefixName + utostr(HashCombinedVal);
  FunctionCallee FCallee = M->getOrInsertFunction(Name, FTy);
  auto *F = cast<Function>(FCallee.getCallee());
  F->setLinkage(GlobalValue::PrivateLinkage);

  auto *EntryBB = BasicBlock::Create(Ctx, "entry", F);
  ReturnInst::Create(Ctx, EntryBB);
  auto *Callee =
      createDecodingTrampoline(G, Op, &*EntryBB->begin(), Key, StrSz, EI);

  Ctors.push_back(F);
  ToInline.push_back(Callee);
  GVarEncInfo.insert({&G, std::move(EI)});

  return true;
}

void StringEncoding::genRoutines(const Triple &TargetTriple, EncodingInfo &EI,
                                 LLVMContext &Ctx) {
  unsigned NumBuiltinRoutines = getNumEncodeDecodeRoutines();
  std::uniform_int_distribution<size_t> Dist(0, NumBuiltinRoutines - 1);
  size_t Idx = Dist(*RNG);

  EI.EncodeFn = getEncodeRoutine(Idx);

  ExitOnError ExitOnErr("Nested clang invocation failed: ");
  const char *DecodeFn = getDecodeRoutine(Idx);
  std::string Routine = (Twine("extern \"C\" {\n") + DecodeFn + "}\n").str();

  {
    Ctx.setDiscardValueNames(false);
    EI.TM = ExitOnErr(
        generateModule(Routine, TargetTriple, "cpp", Ctx,
                       {"-target", TargetTriple.getTriple(), "-std=c++17"}));
    annotateRoutine(*EI.TM);
  }
}

void StringEncoding::annotateRoutine(Module &M) {
  for (Function &F : M) {
    for (BasicBlock &BB : F) {
      for (Instruction &I : BB) {
        if (auto *Load = dyn_cast<LoadInst>(&I))
          addMetadata(*Load, MetaObf(ProtectFieldAccess));
        else if (auto *Store = dyn_cast<StoreInst>(&I))
          addMetadata(*Store, MetaObf(ProtectFieldAccess));
        else if (auto *BinOp = dyn_cast<BinaryOperator>(&I);
                 BinOp && Arithmetic::isSupported(*BinOp))
          addMetadata(*BinOp, MetaObf(OpaqueOp, 2LLU));
      }
    }
  }
}

bool StringEncoding::processLocal(Instruction &I, Use &Op, GlobalVariable &G,
                                  ConstantDataSequential &Data) {
  LLVMContext &Ctx = I.getContext();
  StringRef Str = Data.getRawDataValues();
  uint64_t StrSz = Str.size();
  size_t Max = std::numeric_limits<KeyIntTy>::max();
  std::uniform_int_distribution<KeyIntTy> Dist(1, Max);
  uint64_t Key = Dist(*RNG);

  SDEBUG("Key for {}: 0x{:010x}", Str.str(), Key);

  std::vector<char> Encoded(StrSz);
  EncodingInfo EI(EncodingTy::Local);
  EI.Key = Key;

  genRoutines(Triple(I.getModule()->getTargetTriple()), EI, Ctx);
  EI.EncodeFn(Encoded.data(), Str.data(), Key, StrSz);

  Constant *StrEnc = ConstantDataArray::get(Ctx, Encoded);
  G.setInitializer(StrEnc);

  auto It = GVarEncInfo.insert({&G, std::move(EI)}).first;
  return injectDecodingLocally(I, Op, G, *cast<ConstantDataSequential>(StrEnc),
                               It->getSecond());
}

} // end namespace omvll
