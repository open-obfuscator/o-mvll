//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include <string>

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/CodeGen/StableHashing.h"
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
#include "llvm/Support/Host.h"
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

static const std::vector<std::string> Routines = {
    R"delim(
    void encode(char *out, char *in, unsigned long long key, int size) {
      unsigned char *raw_key = (unsigned char*)(&key);
      for (int i = 0; i < size; ++i) {
        out[i] = in[i] ^ raw_key[i % sizeof(key)];
      }
    }
    void decode(char *out, char *in, unsigned long long key, int size) {
      unsigned char *raw_key = (unsigned char*)(&key);
      for (int i = 0; i < size; ++i) {
        out[i] = in[i] ^ raw_key[i % sizeof(key)];
      }
    }
  )delim",
    R"delim(
    void encode(char *out, char *in, unsigned long long key, int size) {
      unsigned char *raw_key = (unsigned char*)(&key);
      for (int i = 0; i < size; ++i) {
        out[i] = in[i] ^ raw_key[i % sizeof(key)] ^ i;
      }
    }
    void decode(char *out, char *in, unsigned long long key, int size) {
      unsigned char *raw_key = (unsigned char*)(&key);
      for (int i = 0; i < size; ++i) {
        out[i] = in[i] ^ raw_key[i % sizeof(key)] ^ i;
      }
    }
  )delim",
};

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
                         bool IsPartOfStackVariable = false) {
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

  Value *Input = IRB.CreateBitCast(&G, IRB.getInt8PtrTy());
  Value *Output = Input;

  if (IsPartOfStackVariable)
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

  if (!IsPartOfStackVariable)
    return IRB.CreateCall(NewF->getFunctionType(), NewF, Args);

  auto *BoolType = IRB.getInt1Ty();
  GlobalVariable *NeedDecode =
      new GlobalVariable(*M, BoolType, false, GlobalValue::InternalLinkage,
                         ConstantInt::getFalse(BoolType));

  It = IRB.GetInsertPoint();
  auto *WrapperType = FunctionType::get(IRB.getVoidTy(),
                                        {IRB.getInt8PtrTy(), IRB.getInt8PtrTy(),
                                         IRB.getInt8PtrTy(), IRB.getInt64Ty(),
                                         IRB.getInt32Ty()},
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
  bool Changed = false;
  SINFO("[{}] Executing on module {}", name(), M.getName());
  auto &Config = PyConfig::instance();
  ObfuscationConfig *UserConfig = Config.getUserConfig();
  RNG = M.createRNG(name());

  std::vector<Function *> ToVisit;
  for (Function &F : M) {
    if (F.empty() || F.isDeclaration())
      continue;

    demotePHINode(F);
    ToVisit.emplace_back(&F);
  }

  for (Function *F : ToVisit) {
    std::string Name = demangle(F->getName().str());
    SDEBUG("[{}] Visiting function {}", name(), Name);
    Changed |= encodeStrings(*F, *UserConfig);
  }

  // Inline functions.
  for (CallInst *Callee : ToInline) {
    InlineFunctionInfo IFI;
    InlineResult Res = InlineFunction(*Callee, IFI);
    if (!Res.isSuccess())
      fatalError(Res.getFailureReason());
  }

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
  case EncodingTy::Stack:
    return injectOnStack(I, Op, G, Data, Info);
  case EncodingTy::StackLoop:
    return injectOnStackLoop(I, Op, G, Data, Info);
  }

  return false;
}

bool StringEncoding::injectOnStack(Instruction &I, Use &Op, GlobalVariable &G,
                                   ConstantDataSequential &Data,
                                   const StringEncoding::EncodingInfo &Info) {
  auto *Key = std::get_if<KeyBufferTy>(&Info.Key);
  if (!Key)
    fatalError("String stack decoding is expecting a buffer as a key");

  StringRef Str = Data.getRawDataValues();
  uint64_t StrSz = Str.size();

  Use &EncPtr = Op;
  IRBuilder<NoFolder> IRB(I.getParent());
  IRB.SetInsertPoint(&I);

  // Allocate a buffer on the stack that will contain the decoded string.
  AllocaInst *ClearBuffer =
      IRB.CreateAlloca(IRB.getInt8Ty(), IRB.getInt32(StrSz));
  SmallVector<size_t, 20> Indexes(StrSz);

  for (size_t I = 0; I < Indexes.size(); ++I)
    Indexes[I] = I;

  shuffle(Indexes.begin(), Indexes.end(), *RNG);

  for (size_t I = 0; I < StrSz; ++I) {
    size_t J = Indexes[I];
    // Access the char at EncPtr[I].
    Value *EncGEP = IRB.CreateGEP(
        IRB.getInt8Ty(), IRB.CreatePointerCast(EncPtr, IRB.getInt8PtrTy()),
        IRB.getInt32(J));

    // Load the encoded char.
    LoadInst *EncVal = IRB.CreateLoad(IRB.getInt8Ty(), EncGEP);
    addMetadata(*EncVal, MetaObf(ProtectFieldAccess));

    Value *DecodedGEP =
        IRB.CreateGEP(IRB.getInt8Ty(), ClearBuffer, IRB.getInt32(J));
    StoreInst *StoreKey =
        IRB.CreateStore(ConstantInt::get(IRB.getInt8Ty(), (*Key)[J]),
                        DecodedGEP, /* volatile */ true);
    addMetadata(*StoreKey, {
                               MetaObf(ProtectFieldAccess),
                               MetaObf(OpaqueCst),
                           });

    LoadInst *KeyVal = IRB.CreateLoad(IRB.getInt8Ty(), DecodedGEP);
    addMetadata(*KeyVal, MetaObf(ProtectFieldAccess));

    // Decode the value with a xor.
    Value *DecVal = IRB.CreateXor(KeyVal, EncVal);

    if (auto *Op = dyn_cast<Instruction>(DecVal))
      addMetadata(*Op, MetaObf(OpaqueOp, 2LLU));

    // Store the value.
    StoreInst *StoreClear =
        IRB.CreateStore(DecVal, DecodedGEP, /* volatile */ true);
    addMetadata(*StoreClear, MetaObf(ProtectFieldAccess));
  }

  I.setOperand(Op.getOperandNo(), ClearBuffer);
  return true;
}

bool StringEncoding::injectOnStackLoop(
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
  bool Changed =
      std::visit(overloaded{
                     [&](StringEncOptSkip &) { return false; },
                     [&](StringEncOptStack &Stack) {
                       return processOnStack(I, Op, G, Data, Stack);
                     },
                     [&](StringEncOptGlobal &Global) {
                       return processGlobal(I, Op, G, Data, Global);
                     },
                     [&](StringEncOptReplace &Rep) {
                       return processReplace(I, Op, G, Data, Rep);
                     },
                     [&](StringEncOptDefault &) {
                       if (Data.getElementByteSize() < 20) {
                         StringEncOptStack Stack{6};
                         return processOnStack(I, Op, G, Data, Stack);
                       }
                       StringEncOptGlobal Global;
                       return processGlobal(I, Op, G, Data, Global);
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
                                   ConstantDataSequential &Data,
                                   StringEncOptGlobal &Global) {
  Module *M = I.getModule();
  LLVMContext &Ctx = I.getContext();
  StringRef Str = Data.getRawDataValues();
  uint64_t StrSz = Str.size();
  size_t Max = std::numeric_limits<KeyIntTy>::max();
  std::uniform_int_distribution<KeyIntTy> Dist(1, Max);
  uint64_t Key = Dist(*RNG);

  SDEBUG("Key for {}: 0x{:010x}", Str.str(), Key);

  std::vector<uint8_t> Encoded(StrSz);
  EncodingInfo EI(EncodingTy::Global);
  EI.Key = Key;
  genRoutines(Triple(M->getTargetTriple()), EI, Ctx);

  auto JIT = StringEncoding::HostJIT->compile(*EI.HM);
  if (auto EncodeSym = JIT->lookup("encode")) {
    auto EncodeFn = reinterpret_cast<EncRoutineFn>(EncodeSym->getValue());
    EncodeFn(Encoded.data(), Str.data(), Key, StrSz);
  } else {
    fatalError("Cannot find encode routine");
  }

  Constant *StrEnc = ConstantDataArray::get(Ctx, Encoded);
  G.setConstant(false);
  G.setInitializer(StrEnc);

  // Now create a module constructor for the encoded string.
  FunctionType *FTy = FunctionType::get(Type::getVoidTy(Ctx), /* no args */ {},
                                        /* no var args */ false);

  unsigned GlobalIDHashVal =
      stable_hash_combine_string(G.getGlobalIdentifier());
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
  Triple HostTriple(sys::getProcessTriple());
  if (!HostJIT)
    HostJIT = new Jitter(HostTriple.getTriple());

  std::uniform_int_distribution<size_t> Dist(0, Routines.size() - 1);
  size_t Idx = Dist(*RNG);

  ExitOnError ExitOnErr("Nested clang invocation failed: ");
  std::string Routine =
      (Twine("extern \"C\" {\n") + Routines[Idx] + "}\n").str();

  {
    EI.HM = ExitOnErr(generateModule(Routine, HostTriple, "cpp",
                                     HostJIT->getContext(), {"-std=c++17"}));
  }

  {
    Ctx.setDiscardValueNames(false);
    Ctx.setOpaquePointers(true);
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

bool StringEncoding::processOnStackLoop(Instruction &I, Use &Op,
                                        GlobalVariable &G,
                                        ConstantDataSequential &Data) {
  LLVMContext &Ctx = I.getContext();
  StringRef Str = Data.getRawDataValues();
  uint64_t StrSz = Str.size();
  size_t Max = std::numeric_limits<KeyIntTy>::max();
  std::uniform_int_distribution<KeyIntTy> Dist(1, Max);
  uint64_t Key = Dist(*RNG);

  SDEBUG("Key for {}: 0x{:010x}", Str.str(), Key);

  std::vector<uint8_t> Encoded(StrSz);
  EncodingInfo EI(EncodingTy::StackLoop);
  EI.Key = Key;

  genRoutines(Triple(I.getModule()->getTargetTriple()), EI, Ctx);

  auto JIT = StringEncoding::HostJIT->compile(*EI.HM);
  if (auto EncodeSym = JIT->lookup("encode")) {
    auto EncodeFn = reinterpret_cast<EncRoutineFn>(EncodeSym->getValue());
    EncodeFn(Encoded.data(), Str.data(), Key, StrSz);
  } else {
    fatalError("Cannot find encode routine");
  }

  Constant *StrEnc = ConstantDataArray::get(Ctx, Encoded);
  G.setInitializer(StrEnc);

  auto It = GVarEncInfo.insert({&G, std::move(EI)}).first;
  return injectOnStackLoop(I, Op, G, *cast<ConstantDataSequential>(StrEnc),
                           It->getSecond());
}

bool StringEncoding::processOnStack(Instruction &I, Use &Op, GlobalVariable &G,
                                    ConstantDataSequential &Data,
                                    const StringEncOptStack &Stack) {
  StringRef Str = Data.getRawDataValues();
  uint64_t StrSz = Str.size();
  std::uniform_int_distribution<uint8_t> Dist(1, 254);

  SDEBUG("[{}] {}: {}", name(), I.getFunction()->getName(),
         Data.isCString() ? Data.getAsCString() : "<encoded>");

  if (StrSz >= Stack.LoopThreshold)
    return processOnStackLoop(I, Op, G, Data);

  std::vector<uint8_t> Encoded(StrSz);
  std::vector<uint8_t> Key(StrSz);
  std::generate(std::begin(Key), std::end(Key),
                [&Dist, this]() { return Dist(*RNG); });

  for (size_t I = 0; I < StrSz; ++I)
    Encoded[I] = static_cast<uint8_t>(Str[I]) ^ static_cast<uint8_t>(Key[I]);

  Constant *StrEnc = ConstantDataArray::get(I.getContext(), Encoded);
  G.setInitializer(StrEnc);

  EncodingInfo EI(EncodingTy::Stack);
  EI.Key = std::move(Key);
  auto It = GVarEncInfo.insert({&G, std::move(EI)}).first;
  return injectOnStack(I, Op, G, *cast<ConstantDataSequential>(StrEnc),
                       It->getSecond());
}

} // end namespace omvll
