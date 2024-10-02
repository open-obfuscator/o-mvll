#include "omvll/passes/string-encoding/StringEncoding.hpp"

#include "omvll/log.hpp"
#include "omvll/utils.hpp"
#include "omvll/PyConfig.hpp"
#include "omvll/visitvariant.hpp"
#include "omvll/ObfuscationConfig.hpp"
#include "omvll/passes/Metadata.hpp"
#include "omvll/Jitter.hpp"
#include "omvll/passes/arithmetic/Arithmetic.hpp"

#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/CodeGen/StableHashing.h>
#include <llvm/Demangle/Demangle.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/NoFolder.h>
#include <llvm/Support/Host.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/Local.h>
#include <llvm/Transforms/Utils/ModuleUtils.h>

#include <string>

static llvm::ExitOnError ExitOnErr;

using namespace llvm;

namespace omvll {

static const std::vector<std::string> ROUTINES = {
  R"delim(
    void encode(char* out, char* in, unsigned long long key, int size) {
      unsigned char* raw_key = (unsigned char*)(&key);
      for (int i = 0; i < size; ++i) {
        out[i] = in[i] ^ raw_key[i % sizeof(key)];
      }
      return;
    }
    void decode(char* out, char* in, unsigned long long key, int size) {
      unsigned char* raw_key = (unsigned char*)(&key);
      for (int i = 0; i < size; ++i) {
        out[i] = in[i] ^ raw_key[i % sizeof(key)];
      }
    }
  )delim",
  R"delim(
    void encode(char* out, char* in, unsigned long long key, int size) {
      unsigned char* raw_key = (unsigned char*)(&key);
      for (int i = 0; i < size; ++i) {
        out[i] = in[i] ^ raw_key[i % sizeof(key)] ^ i;
      }
      return;
    }
    void decode(char* out, char* in, unsigned long long key, int size) {
      unsigned char* raw_key = (unsigned char*)(&key);
      for (int i = 0; i < size; ++i) {
        out[i] = in[i] ^ raw_key[i % sizeof(key)] ^ i;
      }
    }
  )delim",
};

inline bool isEligible(const GlobalVariable& G) {
  if (G.isNullValue() || G.isZeroValue())
    return false;

  if (!(G.isConstant() && G.hasInitializer()))
    return false;

  return true;
}

inline bool isEligible(const ConstantDataSequential& CDS) {
  return CDS.isCString() && CDS.getAsCString().size() > 1;
}

inline bool isSkip(const StringEncodingOpt& encInfo) {
  return std::get_if<StringEncOptSkip>(&encInfo) != nullptr;
}

GlobalVariable *extractGlobalVariable(ConstantExpr *Expr) {
  while (Expr) {
    if (Expr->getOpcode() == llvm::Instruction::IntToPtr ||
        llvm::Instruction::isBinaryOp(Expr->getOpcode())) {
      Expr = dyn_cast<ConstantExpr>(Expr->getOperand(0));
    } else if (Expr->getOpcode() == llvm::Instruction::PtrToInt ||
               Expr->getOpcode() == llvm::Instruction::GetElementPtr) {
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
  AllocaInst *ClearBuffer =
      IRB.CreateAlloca(ArrayType::get(IRB.getInt8Ty(), Size));

  AllocaInst *Key = IRB.CreateAlloca(IRB.getInt64Ty());
  AllocaInst *StrSize = IRB.CreateAlloca(IRB.getInt32Ty());

  IRB.SetInsertPoint(NewPt);
  auto *StoreKey = IRB.CreateStore(IRB.getInt64(KeyValI64), Key);
  addMetadata(*StoreKey, MetaObf(OPAQUE_CST));

  IRB.CreateStore(IRB.getInt32(Size), StrSize);

  LoadInst *LoadKey = IRB.CreateLoad(IRB.getInt64Ty(), Key);
  addMetadata(*LoadKey, MetaObf(OPAQUE_CST));

  auto *KeyVal = IRB.CreateBitCast(LoadKey, IRB.getInt64Ty());

  LoadInst *LStrSize = IRB.CreateLoad(IRB.getInt32Ty(), StrSize);
  addMetadata(*LStrSize, MetaObf(OPAQUE_CST));

  auto *VStrSize = IRB.CreateBitCast(LStrSize, IRB.getInt32Ty());

  Function *FDecode = EI.TM->getFunction("decode");
  if (FDecode == nullptr)
    fatalError("Can't find the 'decode' routine");

  // TODO: support ObjC strings as well
  if (isa<ConstantExpr>(EncPtr))
    assert(extractGlobalVariable(cast<ConstantExpr>(EncPtr)) == &G &&
           "Previously extracted global variable need to match");

  Value *Input = IRB.CreateBitCast(&G, IRB.getInt8PtrTy());
  Value *Output = Input;

  if (IsPartOfStackVariable)
    Output = IRB.CreateInBoundsGEP(ClearBuffer->getAllocatedType(), ClearBuffer,
                                   {IRB.getInt64(0), IRB.getInt64(0)});

  auto *NewF =
      Function::Create(FDecode->getFunctionType(), GlobalValue::PrivateLinkage,
                       "__omvll_decode", NewPt->getModule());

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
    if (E != V) {
      std::string err = fmt::format("Args #{}: Expecting {} while it is {}",
                                    Idx, ToString(*E), ToString(*V));
      fatalError(err.c_str());
    }
  }

  if (IsPartOfStackVariable) {
    if (auto *CE = dyn_cast<ConstantExpr>(EncPtr)) {
      auto [First, Last] = materializeConstantExpression(NewPt, CE);
      assert(
          ((First != Last) ||
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
  }

  return IRB.CreateCall(NewF->getFunctionType(), NewF, Args);
}

bool StringEncoding::runOnBasicBlock(Module &M, Function &F, BasicBlock &BB,
                                     ObfuscationConfig &userConfig) {
  bool Changed = false;

  for (Instruction &I : BB) {
    if (isa<PHINode>(I)) {
      SWARN("{} contains Phi node which could raise issues!", demangle(F.getName().str()));
      continue;
    }

    for (Use& Op : I.operands()) {
      GlobalVariable *G = dyn_cast<GlobalVariable>(Op->stripPointerCasts());

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
      if (Data == nullptr)
        continue;

      // Create a default option which skips the encoding.
      auto EncInfoOpt = std::make_unique<StringEncodingOpt>(StringEncOptSkip());

      if (EncodingInfo* EI = getEncoding(*G)) {
        // The global variable is already encoded.
        // Let's check if we should insert a decoding stub.
        Changed |= injectDecoding(BB, I, Op, *G, *Data, *EI);
        continue;
      }

      if (isEligible(*Data)) {
        // Get the encoding type from the user.
        EncInfoOpt = std::make_unique<StringEncodingOpt>(
            userConfig.obfuscate_string(&M, &F, Data->getAsCString().str()));
      }

      if (isSkip(*EncInfoOpt) ||
          (MaybeStringInCEInitializer &&
           std::get_if<StringEncOptGlobal>(EncInfoOpt.get()) == nullptr))
        continue;

      SINFO("[{}] Processing string {}", name(), Data->getAsCString());
      Changed |= process(BB, I, *ActualOp, *G, *Data, *EncInfoOpt);
    }
  }

  return Changed;
}

PreservedAnalyses StringEncoding::run(Module &M, ModuleAnalysisManager &MAM) {
  SINFO("[{}] Executing on module {}", name(), M.getName());
  bool Changed = false;
  auto &Config = PyConfig::instance();
  ObfuscationConfig *UserConfig = Config.getUserConfig();

  RNG_ = M.createRNG(name());

  std::vector<Function *> FuncsToVisit;
  for (Function &F : M)
    FuncsToVisit.emplace_back(&F);

  for (auto *F : FuncsToVisit) {
    demotePHINode(*F);
    std::string DemangledFName = demangle(F->getName().str());
    SDEBUG("[{}] Visiting function {}", name(), DemangledFName);
    for (BasicBlock &BB : *F) {
      Changed |= runOnBasicBlock(M, *F, BB, *UserConfig);
    }
  }

  // Inline function
  for (CallInst *Callee : inline_wlist_) {
    InlineFunctionInfo IFI;
    InlineResult Res = InlineFunction(*Callee, IFI);
    if (!Res.isSuccess()) {
      fatalError(Res.getFailureReason());
    }
  }
  inline_wlist_.clear();

  // Create CTOR functions
  for (Function *F : ctor_)
    appendToGlobalCtors(M, F, 0);
  ctor_.clear();

  SINFO("[{}] Changes{}applied on module {}", name(), Changed ? " " : " not ",
        M.getName());

  return Changed ? PreservedAnalyses::none() :
                   PreservedAnalyses::all();
}

bool StringEncoding::injectDecoding(BasicBlock &BB, Instruction &I, Use &Op,
                                    GlobalVariable &G,
                                    ConstantDataSequential &data,
                                    const StringEncoding::EncodingInfo &info) {
  switch (info.type) {
    case EncodingTy::NONE:
    case EncodingTy::REPLACE:
    case EncodingTy::GLOBAL:
      return false;

    case EncodingTy::STACK:
      return injectOnStack(BB, I, Op, G, data, info);

    case EncodingTy::STACK_LOOP:
      return injectOnStackLoop(BB, I, Op, G, data, info);
  }

  return false;
}

bool StringEncoding::injectOnStack(BasicBlock &BB, Instruction &I, Use &Op,
                                   GlobalVariable &G,
                                   ConstantDataSequential &data,
                                   const StringEncoding::EncodingInfo &info) {
  auto *Key = std::get_if<key_buffer_t>(&info.key);
  if (Key == nullptr)
    fatalError("String stack decoding is expecting a buffer as a key");

  StringRef Str = data.getRawDataValues();
  uint64_t StrSz = Str.size();

  Use& EncPtr = Op;
  IRBuilder<NoFolder> IRB(&BB);
  IRB.SetInsertPoint(&I);

  // Allocate a buffer on the stack that will contain the decoded string
  AllocaInst *ClearBuffer =
      IRB.CreateAlloca(IRB.getInt8Ty(), IRB.getInt32(StrSz));
  llvm::SmallVector<size_t, 20> Indexes(StrSz);

  for (size_t I = 0; I < Indexes.size(); ++I)
    Indexes[I] = I;

  shuffle(Indexes.begin(), Indexes.end(), *RNG_);

  for (size_t I = 0; I < StrSz; ++I) {
    size_t J = Indexes[I];
    // Access the char at EncPtr[i]
    Value *EncGEP = IRB.CreateGEP(
        IRB.getInt8Ty(), IRB.CreatePointerCast(EncPtr, IRB.getInt8PtrTy()),
        IRB.getInt32(J));

    // Load the encoded char
    LoadInst *EncVal = IRB.CreateLoad(IRB.getInt8Ty(), EncGEP);
    addMetadata(*EncVal, MetaObf(PROTECT_FIELD_ACCESS));

    Value *DecodedGEP =
        IRB.CreateGEP(IRB.getInt8Ty(), ClearBuffer, IRB.getInt32(J));
    StoreInst *StoreKey =
        IRB.CreateStore(ConstantInt::get(IRB.getInt8Ty(), (*Key)[J]),
                        DecodedGEP, /* volatile */ true);

    addMetadata(*StoreKey, {
                               MetaObf(PROTECT_FIELD_ACCESS),
                               MetaObf(OPAQUE_CST),
                           });

    LoadInst *KeyVal = IRB.CreateLoad(IRB.getInt8Ty(), DecodedGEP);
    addMetadata(*KeyVal, MetaObf(PROTECT_FIELD_ACCESS));

    // Decode the value with a xor
    Value *DecVal = IRB.CreateXor(KeyVal, EncVal);

    if (auto *Op = dyn_cast<Instruction>(DecVal))
      addMetadata(*Op, MetaObf(OPAQUE_OP, 2llu));

    // Store the value
    StoreInst *StoreClear =
        IRB.CreateStore(DecVal, DecodedGEP, /* volatile */ true);
    addMetadata(*StoreClear, MetaObf(PROTECT_FIELD_ACCESS));
  }

  I.setOperand(Op.getOperandNo(), ClearBuffer);
  return true;
}

bool StringEncoding::injectOnStackLoop(
    BasicBlock &BB, Instruction &I, Use &Op, GlobalVariable &G,
    ConstantDataSequential &data, const StringEncoding::EncodingInfo &info) {
  auto *Key = std::get_if<key_int_t>(&info.key);
  if (Key == nullptr)
    fatalError("String stack decoding loop is expecting a buffer as a key! ");

  uint64_t StrSz = data.getRawDataValues().size();

  SDEBUG("Key for 0x{:010x}", *Key);

  CallInst *Callee =
      createDecodingTrampoline(G, Op, &I, *Key, StrSz, info, true);
  inline_wlist_.push_back(Callee);

  return true;
}

bool StringEncoding::process(BasicBlock &BB, Instruction &I, Use &Op,
                             GlobalVariable &G, ConstantDataSequential &data,
                             StringEncodingOpt &opt) {
  bool Changed = std::visit(overloaded {
      [&] (StringEncOptSkip& )         { return false; },
      [&] (StringEncOptStack& stack)   { return processOnStack(BB, I, Op, G, data, stack); },
      [&] (StringEncOptGlobal& global) { return processGlobal(BB, I, Op, G, data, global); },
      [&] (StringEncOptReplace& rep)   { return processReplace(BB, I, Op, G, data, rep); },
      [&] (StringEncOptDefault&) {
        if (data.getElementByteSize() < 20) {
          StringEncOptStack stack{6};
          return processOnStack(BB, I, Op, G, data, stack);
        }
        StringEncOptGlobal global;
        return processGlobal(BB, I, Op, G, data, global);
      },
  }, opt);
  return Changed;
}

bool StringEncoding::processReplace(BasicBlock &BB, Instruction &, Use &,
                                    GlobalVariable &G,
                                    ConstantDataSequential &data,
                                    StringEncOptReplace &rep) {
  SDEBUG("Replacing {} with {}", data.getAsString().str(), rep.newString);
  const size_t Inlen = rep.newString.size();

  if (rep.newString.size() < data.getNumElements()) {
    SWARN("'{}' is smaller than the original string. It will be padded with 0", rep.newString);
    rep.newString =
        rep.newString.append(data.getNumElements() - Inlen - 1, '\0');
  } else {
    SWARN("'{}' is lager than the original string. It will be shrinked to fit the original data", rep.newString);
    rep.newString = rep.newString.substr(0, data.getNumElements() - 1);
  }

  Constant *NewVal =
      ConstantDataArray::getString(BB.getContext(), rep.newString);
  G.setInitializer(NewVal);
  gve_info_.insert({&G, EncodingInfo(EncodingTy::REPLACE)});
  return true;
}

bool StringEncoding::processGlobal(BasicBlock &BB, Instruction &I, Use &Op,
                                   GlobalVariable &G,
                                   ConstantDataSequential &data,
                                   StringEncOptGlobal &global) {
  Module *M = BB.getModule();
  StringRef Str = data.getRawDataValues();
  uint64_t StrSz = Str.size();
  std::uniform_int_distribution<key_int_t> Dist(1, std::numeric_limits<key_int_t>::max());
  uint64_t Key = Dist(*RNG_);

  SDEBUG("Key for {}: 0x{:010x}", Str.str(), Key);

  std::vector<uint8_t> Encoded(StrSz);
  EncodingInfo EI(EncodingTy::GLOBAL);
  EI.key = Key;
  genRoutines(Triple(BB.getModule()->getTargetTriple()), EI, BB.getContext());

  auto JIT = StringEncoding::HOSTJIT->compile(*EI.HM);
  if (auto E = JIT->lookup("encode")) {
    auto Enc = reinterpret_cast<enc_routine_t>(E->getValue());
    Enc(Encoded.data(), Str.data(), Key, StrSz);
  } else {
    fatalError("Can't find 'encode' in the routine");
  }

  Constant *StrEnc = ConstantDataArray::get(BB.getContext(), Encoded);
  G.setConstant(false);
  G.setInitializer(StrEnc);
  // Now create a module constructor for the encoded string
  // FType: void decode();
  LLVMContext& Ctx = BB.getContext();
  FunctionType* FTy = FunctionType::get(Type::getVoidTy(BB.getContext()),
                                        /* no args */{}, /* no var args */ false);
  unsigned GlobalIDHashVal =
      llvm::stable_hash_combine_string(G.getGlobalIdentifier());
  unsigned HashCombinedVal =
      llvm::stable_hash_combine(GlobalIDHashVal, StrSz, Key);
  std::string Name = "__omvll_ctor_" + utostr(HashCombinedVal);
  FunctionCallee FCallee = M->getOrInsertFunction(Name, FTy);
  auto* F = cast<Function>(FCallee.getCallee());
  F->setLinkage(llvm::GlobalValue::PrivateLinkage);

  auto *EntryBB = BasicBlock::Create(Ctx, "entry", F);
  ReturnInst::Create(Ctx, EntryBB);
  CallInst *Callee =
      createDecodingTrampoline(G, Op, &*EntryBB->begin(), Key, StrSz, EI);

  ctor_.push_back(F);
  inline_wlist_.push_back(Callee);

  gve_info_.insert({&G, std::move(EI)});
  return true;
}

void StringEncoding::genRoutines(const Triple &TargetTriple, EncodingInfo &EI,
                                 LLVMContext &Ctx) {
  Triple HostTriple(sys::getProcessTriple());

  if (HOSTJIT == nullptr)
    HOSTJIT = new Jitter(HostTriple.getTriple());

  std::uniform_int_distribution<size_t> Dist(0, ROUTINES.size() - 1);
  size_t idx = Dist(*RNG_);
  StringRef R = ROUTINES[idx];

  ExitOnError ExitOnErr("Nested clang invocation failed: ");
  std::string Routine = (Twine("extern \"C\" {\n") + R + "}\n").str();

  {
    EI.HM = ExitOnErr(
        generateModule(Routine, HostTriple, "cpp", HOSTJIT->getContext(),
                       {"-std=c++17", "-mllvm", "--opaque-pointers"}));
  }

  {
    Ctx.setDiscardValueNames(false);
    EI.TM = ExitOnErr(
        generateModule(Routine, TargetTriple, "cpp", Ctx,
                       {"-target", TargetTriple.getTriple(), "-std=c++17",
                        "-mllvm", "--opaque-pointers"}));
    annotateRoutine(*EI.TM);
  }
}

void StringEncoding::annotateRoutine(llvm::Module& M) {
  for (Function& F : M) {
    for (BasicBlock& BB : F) {
      for (Instruction& I : BB) {
        if (auto* Load = dyn_cast<LoadInst>(&I)) {
          addMetadata(*Load, MetaObf(PROTECT_FIELD_ACCESS));
        }
        else if (auto* Store = dyn_cast<StoreInst>(&I)) {
          addMetadata(*Store, MetaObf(PROTECT_FIELD_ACCESS));
        }
        else if (auto* BinOp = dyn_cast<BinaryOperator>(&I)) {
            if (Arithmetic::isSupported(*BinOp)) {
              addMetadata(*BinOp, MetaObf(OPAQUE_OP, 2llu));
            }
        }
      }
    }
  }
}

bool StringEncoding::processOnStackLoop(BasicBlock& BB, Instruction& I, Use& Op, GlobalVariable& G,
                                        ConstantDataSequential& data)
{
  StringRef Str = data.getRawDataValues();
  uint64_t StrSz = Str.size();
  std::uniform_int_distribution<key_int_t> Dist(1, std::numeric_limits<key_int_t>::max());
  uint64_t key = Dist(*RNG_);

  SDEBUG("Key for {}: 0x{:010x}", Str.str(), key);

  std::vector<uint8_t> Encoded(StrSz);
  EncodingInfo EI(EncodingTy::STACK_LOOP);
  EI.key = key;

  genRoutines(Triple(BB.getModule()->getTargetTriple()), EI, BB.getContext());

  auto JIT = StringEncoding::HOSTJIT->compile(*EI.HM);
  if (auto E = JIT->lookup("encode")) {
    auto Enc = reinterpret_cast<enc_routine_t>(E->getValue());
    Enc(Encoded.data(), Str.data(), key, StrSz);
  } else {
    fatalError("Can't find 'encode' in the routine");
  }

  Constant *StrEnc = ConstantDataArray::get(BB.getContext(), Encoded);
  G.setInitializer(StrEnc);

  auto It = gve_info_.insert({&G, std::move(EI)}).first;
  return injectOnStackLoop(
      BB, I, Op, G, *dyn_cast<ConstantDataSequential>(StrEnc), It->getSecond());
}

bool StringEncoding::processOnStack(BasicBlock &BB, Instruction &I, Use &Op,
                                    GlobalVariable &G,
                                    ConstantDataSequential &data,
                                    const StringEncOptStack &stack) {
  std::uniform_int_distribution<uint8_t> Dist(1, 254);
  StringRef Str = data.getRawDataValues();
  uint64_t StrSz = Str.size();

  SDEBUG("[{}] {}: {}", name(), BB.getParent()->getName(),
                        data.isCString() ? data.getAsCString() : "<encoded>");

  if (StrSz >= stack.loopThreshold)
    return processOnStackLoop(BB, I, Op, G, data);

  std::vector<uint8_t> Encoded(StrSz);
  std::vector<uint8_t> Key(StrSz);
  std::generate(std::begin(Key), std::end(Key),
                [&Dist, this]() { return Dist(*RNG_); });

  for (size_t I = 0; I < StrSz; ++I)
    Encoded[I] = static_cast<uint8_t>(Str[I]) ^ static_cast<uint8_t>(Key[I]);

  Constant *StrEnc = ConstantDataArray::get(BB.getContext(), Encoded);
  G.setInitializer(StrEnc);

  EncodingInfo EI(EncodingTy::STACK);
  EI.key = std::move(Key);
  auto It = gve_info_.insert({&G, std::move(EI)}).first;
  return injectOnStack(BB, I, Op, G, *dyn_cast<ConstantDataSequential>(StrEnc),
                       It->getSecond());
}

} // namespace omvll
