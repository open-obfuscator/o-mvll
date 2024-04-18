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
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/CodeGen/StableHashing.h>
#include <llvm/Demangle/Demangle.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/NoFolder.h>
#include <llvm/Support/Host.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/ModuleUtils.h>
#include <string>

#include "llvm/ExecutionEngine/Orc/LLJIT.h"

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
  if (G.isNullValue() || G.isZeroValue()) {
    return false;
  }
  if (!(G.isConstant() && G.hasInitializer())) {
    return false;
  }
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

bool StringEncoding::runOnBasicBlock(Module& M, Function& F, BasicBlock& BB,
                                     ObfuscationConfig& userConfig)
{
  bool Changed = false;

  for (Instruction &I : BB) {
    //SDEBUG("{}", ToString(I));
    if (isa<PHINode>(I)) {
      SWARN("{} contains Phi node which could raise issues!", demangle(F.getName().str()));
      continue;
    }

    for (Use& Op : I.operands()) {
      GlobalVariable *G = dyn_cast<GlobalVariable>(Op->stripPointerCasts());

      if (!G) {
        if (auto *CE = dyn_cast<ConstantExpr>(Op))
          G = extractGlobalVariable(CE);
      }

      if (!G)
        continue;

      if (!isEligible(*G)) {
        continue;
      }


      auto* data = dyn_cast<ConstantDataSequential>(G->getInitializer());
      if (data == nullptr) {
        continue;
      }

      // Create a default option which skips the encoding
      auto encInfo = std::make_unique<StringEncodingOpt>(StringEncOptSkip());

      if (EncodingInfo* EI = getEncoding(*G)) {
        // The global variable is already encoded. Let's check if we should insert a decoding stub
        Changed |= injectDecoding(BB, I, Op, *G, *data, *EI);
        continue;
      }

      if (isEligible(*data)) {
        // Get the encoding type from the user
        encInfo = std::make_unique<StringEncodingOpt>(userConfig.obfuscate_string(&M, &F, data->getAsCString().str()));
      }

      if (isSkip(*encInfo)) {
        continue;
      }

      Changed |= process(BB, I, Op, *G, *data, *encInfo);
    }
  }
  return Changed;
}

PreservedAnalyses StringEncoding::run(Module &M, ModuleAnalysisManager &MAM) {

  bool Changed = false;
  auto& config = PyConfig::instance();
  ObfuscationConfig* userConfig = config.getUserConfig();

  //ObfuscationAnalysis::Result& R = ObfuscationAnalysis::GetOrSet(M, MAM);

  RNG_ = M.createRNG(name());
  SDEBUG("[{}] Module: {}", name(), M.getSourceFileName());

  for (Function &F : M) {
    demotePHINode(F);
    StringRef name = F.getName();
    std::string demangled = demangle(name.str());
    for (BasicBlock &BB : F) {
      Changed |= runOnBasicBlock(M, F, BB, *userConfig);
    }
  }

  // Inline function
  for (CallInst* callee : inline_wlist_) {
    InlineFunctionInfo IFI;
    InlineResult res = InlineFunction(*callee, IFI);
    if (!res.isSuccess()) {
      fatalError(res.getFailureReason());
    }
  }
  inline_wlist_.clear();

  // Create CTOR functions
  for (Function* F : ctor_) {
    appendToGlobalCtors(M, F, 0);
  }
  ctor_.clear();
  SINFO("[{}] Done!", name());
  return Changed ? PreservedAnalyses::none() :
                   PreservedAnalyses::all();
}

bool StringEncoding::injectDecoding(BasicBlock& BB, Instruction& I, Use& Op, GlobalVariable& G,
                                    ConstantDataSequential& data, const StringEncoding::EncodingInfo& info)
{
  switch (info.type) {
    case EncodingTy::NONE:
    case EncodingTy::REPLACE:
    case EncodingTy::GLOBAL:
      {
        return false;
      }

    case EncodingTy::STACK:
      {
        return injectOnStack(BB, I, Op, G, data, info);
      }

    case EncodingTy::STACK_LOOP:
      {
        return injectOnStackLoop(BB, I, Op, G, data, info);
      }
  }
  return false;
}

bool StringEncoding::injectOnStack(BasicBlock& BB, Instruction& I, Use& Op, GlobalVariable& G,
                                   ConstantDataSequential& data, const StringEncoding::EncodingInfo& info)
{

  auto* key = std::get_if<key_buffer_t>(&info.key);
  if (key == nullptr) {
    fatalError("String stack decoding is expecting a buffer as a key");
  }

  StringRef str = data.getRawDataValues();

  Use& EncPtr = Op;
  IRBuilder<NoFolder> IRB(&BB);
  IRB.SetInsertPoint(&I);

  // Allocate a buffer on the stack that will contain the decoded string
  AllocaInst* clearBuffer = IRB.CreateAlloca(IRB.getInt8Ty(),
                                             IRB.getInt32(str.size()));
  llvm::SmallVector<size_t, 20> indexes(str.size());

  for (size_t i = 0; i < indexes.size(); ++i) {
    indexes[i] = i;
  }
  shuffle(indexes.begin(), indexes.end(), *RNG_);

  for (size_t i = 0; i < str.size(); ++i) {
    size_t j = indexes[i];
    // Access the char at EncPtr[i]
    Value *encGEP = IRB.CreateGEP(IRB.getInt8Ty(),
                                  IRB.CreatePointerCast(EncPtr, IRB.getInt8PtrTy()),
                                  IRB.getInt32(j));

    // Load the encoded char
    LoadInst* encVal = IRB.CreateLoad(IRB.getInt8Ty(), encGEP);
    addMetadata(*encVal, MetaObf(PROTECT_FIELD_ACCESS));

    Value* decodedGEP = IRB.CreateGEP(IRB.getInt8Ty(), clearBuffer, IRB.getInt32(j));
    StoreInst* storeKey = IRB.CreateStore(ConstantInt::get(IRB.getInt8Ty(), (*key)[j]),
                                          decodedGEP, /* volatile */true);

    addMetadata(*storeKey, {
                  MetaObf(PROTECT_FIELD_ACCESS),
                  MetaObf(OPAQUE_CST),
                });

    LoadInst* keyVal = IRB.CreateLoad(IRB.getInt8Ty(), decodedGEP);
    addMetadata(*keyVal, MetaObf(PROTECT_FIELD_ACCESS));

    // Decode the value with a xor
    Value* decVal = IRB.CreateXor(keyVal, encVal);

    if (auto* Op = dyn_cast<Instruction>(decVal)) {
      addMetadata(*Op, MetaObf(OPAQUE_OP, 2llu));
    }

    // Store the value
    StoreInst* storeClear = IRB.CreateStore(decVal, decodedGEP, /* volatile */true);
    addMetadata(*storeClear, MetaObf(PROTECT_FIELD_ACCESS));
  }

  I.setOperand(Op.getOperandNo(), clearBuffer);
  return true;
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

bool StringEncoding::injectOnStackLoop(BasicBlock& BB, Instruction& I, Use& Op, GlobalVariable& G,
                                       ConstantDataSequential& data, const StringEncoding::EncodingInfo& info)
{
  auto* key = std::get_if<key_int_t>(&info.key);
  if (key == nullptr) {
    fatalError("String stack decoding loop is expecting a buffer as a key! ");
  }

  StringRef str = data.getRawDataValues();

  SDEBUG("Key for 0x{:010x}", *key);

  Use& EncPtr = Op;
  IRBuilder<NoFolder> IRB(&BB);
  IRB.SetInsertPoint(&I);

  AllocaInst *ClearBuffer =
      IRB.CreateAlloca(ArrayType::get(IRB.getInt8Ty(), str.size()));
  auto *CastClearBuffer = IRB.CreateBitCast(ClearBuffer, IRB.getInt8PtrTy());

  AllocaInst* Key     = IRB.CreateAlloca(IRB.getInt64Ty());
  AllocaInst* StrSize = IRB.CreateAlloca(IRB.getInt32Ty());

  auto* StoreKey = IRB.CreateStore(IRB.getInt64(*key), Key);
  addMetadata(*StoreKey, MetaObf(OPAQUE_CST));

  IRB.CreateStore(IRB.getInt32(str.size()), StrSize);

  LoadInst* LoadKey = IRB.CreateLoad(IRB.getInt64Ty(), Key);
  addMetadata(*LoadKey, MetaObf(OPAQUE_CST));

  auto* KeyVal = IRB.CreateBitCast(LoadKey, IRB.getInt64Ty());

  LoadInst* LStrSize = IRB.CreateLoad(IRB.getInt32Ty(), StrSize);
  addMetadata(*LStrSize, MetaObf(OPAQUE_CST));

  auto* VStrSize = IRB.CreateBitCast(LStrSize, IRB.getInt32Ty());

  Function* FDecode = info.TM->getFunction("decode");
  if (FDecode == nullptr) {
    fatalError("Can't find the 'decode' routine");
  }

  // TODO: support ObjC strings as well
  Value *CastEncPtr = nullptr;
  if (isa<GlobalVariable>(EncPtr) || isa<ConstantExpr>(EncPtr)) {
    if (isa<ConstantExpr>(EncPtr))
      assert(extractGlobalVariable(cast<ConstantExpr>(EncPtr)) == &G &&
             "Previously extracted global variable need to match");
    CastEncPtr = IRB.CreateBitCast(&G, IRB.getInt8PtrTy());
  }

  auto *M = BB.getModule();
  auto *NewF =
      Function::Create(FDecode->getFunctionType(), GlobalValue::PrivateLinkage,
                       "__omvll_decode", M);

  ValueToValueMapTy VMap;
  auto NewFArgsIt = NewF->arg_begin();
  auto FArgsIt = FDecode->arg_begin();
  for (auto FArgsEnd = FDecode->arg_end(); FArgsIt != FArgsEnd; ++NewFArgsIt, ++FArgsIt) {
    VMap[&*FArgsIt] = &*NewFArgsIt;
  }
  SmallVector<ReturnInst*, 8> Returns;
  CloneFunctionInto(NewF, FDecode, VMap, CloneFunctionChangeType::DifferentModule, Returns);
  NewF->setDSOLocal(true);

  std::vector<Value *> Args = {
      CastClearBuffer, CastEncPtr ? CastEncPtr : EncPtr, KeyVal, VStrSize};

  if (NewF->arg_size() != 4) {
    fatalError(fmt::format("Expecting 4 arguments ({} provided)", NewF->arg_size()));
  }

  for (size_t i = 0; i < NewF->arg_size(); ++i) {
    auto* E = NewF->getFunctionType()->getParamType(i);
    auto* V = Args[i]->getType();
    if (E != V) {
      std::string err = fmt::format("Args #{}: Expecting {} while it is {}",
                                    i, ToString(*E), ToString(*V));
      fatalError(err.c_str());
    }
  }

  CallInst* callee = IRB.CreateCall(NewF->getFunctionType(), NewF, Args);
  inline_wlist_.push_back(callee);

  if (auto *CE = dyn_cast<ConstantExpr>(EncPtr)) {
    auto [First, Last] = materializeConstantExpression(&I, CE);
    assert(((First != Last) ||
            (isa<GetElementPtrInst>(First) || isa<PtrToIntInst>(First))) &&
           "Nested constantexpr in getelementptr/ptrtoint should not appear?");
    Last->setOperand(0, ClearBuffer);
    I.setOperand(Op.getOperandNo(), First);
  } else {
    I.setOperand(Op.getOperandNo(), ClearBuffer);
  }

  return true;
}


bool StringEncoding::process(BasicBlock& BB, Instruction& I, Use& Op, GlobalVariable& G,
                             ConstantDataSequential& data, StringEncodingOpt& opt)
{
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

bool StringEncoding::processReplace(BasicBlock& BB, Instruction&, Use&, GlobalVariable& G,
                                    ConstantDataSequential& data, StringEncOptReplace& rep)
{
  SDEBUG("Replacing {} with {}", data.getAsString().str(), rep.newString);
  const size_t inlen = rep.newString.size();
  if (rep.newString.size() < data.getNumElements()) {
    SWARN("'{}' is smaller than the original string. It will be padded with 0", rep.newString);
    rep.newString = rep.newString.append(data.getNumElements() - inlen - 1, '\0');
  } else {
    SWARN("'{}' is lager than the original string. It will be shrinked to fit the original data", rep.newString);
    rep.newString = rep.newString.substr(0, data.getNumElements() - 1);
  }
  Constant* NewVal = ConstantDataArray::getString(BB.getContext(), rep.newString);

  G.setInitializer(NewVal);
  gve_info_.insert({&G, EncodingInfo(EncodingTy::REPLACE)});
  return true;
}

bool StringEncoding::processGlobal(BasicBlock& BB, Instruction&, Use& Op, GlobalVariable& G,
                                   ConstantDataSequential& data, StringEncOptGlobal& global)
{

  G.setConstant(false);
  StringRef str = data.getRawDataValues();
  std::uniform_int_distribution<key_int_t> Dist(1, std::numeric_limits<key_int_t>::max());
  uint64_t key = Dist(*RNG_);
  Use& EncPtr = Op;

  SDEBUG("Key for {}: 0x{:010x}", str.str(), key);

  std::vector<uint8_t> encoded(str.size());
  EncodingInfo EI(EncodingTy::GLOBAL);
  EI.key = key;
  genRoutines(Triple(BB.getModule()->getTargetTriple()), EI, BB.getContext());

  auto JIT = StringEncoding::HOSTJIT->compile(*EI.HM);
  if (auto E = JIT->lookup("encode")) {
    auto enc = reinterpret_cast<enc_routine_t>(E->getAddress());
    enc(encoded.data(), str.data(), key, str.size());
  } else {
    fatalError("Can't find 'encode' in the routine");
  }

  Constant* StrEnc = ConstantDataArray::get(BB.getContext(), encoded);
  G.setInitializer(StrEnc);
  // Now create a module constructor for the encoded string
  // FType: void decode();
  Module* M = BB.getModule();
  LLVMContext& Ctx = BB.getContext();
  FunctionType* FTy = FunctionType::get(Type::getVoidTy(BB.getContext()),
                                        /* no args */{}, /* no var args */ false);
  unsigned GlobalIDHashVal =
      llvm::stable_hash_combine_string(G.getGlobalIdentifier());
  unsigned HashCombinedVal =
      llvm::stable_hash_combine(GlobalIDHashVal, str.size(), key);
  std::string Name = "__omvll_ctor_" + utostr(HashCombinedVal);
  FunctionCallee FCallee = M->getOrInsertFunction(Name, FTy);
  auto* F = cast<Function>(FCallee.getCallee());
  F->setLinkage(llvm::GlobalValue::PrivateLinkage);

  BasicBlock *EntryBB = BasicBlock::Create(Ctx, "entry", F);

  IRBuilder<NoFolder> IRB(EntryBB);

  AllocaInst* Key     = IRB.CreateAlloca(IRB.getInt64Ty());
  AllocaInst* StrSize = IRB.CreateAlloca(IRB.getInt32Ty());

  StoreInst* StoreKey = IRB.CreateStore(IRB.getInt64(key), Key);
  addMetadata(*StoreKey, MetaObf(OPAQUE_CST));
  IRB.CreateStore(IRB.getInt32(str.size()), StrSize);

  LoadInst* LoadKey = IRB.CreateLoad(IRB.getInt64Ty(), Key);
  auto* KeyVal = IRB.CreateBitCast(LoadKey, IRB.getInt64Ty());

  LoadInst* LStrSize = IRB.CreateLoad(IRB.getInt32Ty(), StrSize);
  auto* VStrSize = IRB.CreateBitCast(LStrSize, IRB.getInt32Ty());

  auto* DataPtr = IRB.CreatePointerCast(EncPtr, IRB.getInt8PtrTy());

  Function* FDecode = EI.TM->getFunction("decode");
  if (FDecode == nullptr) {
    fatalError("Can't find the 'decode' routine");
  }

  auto *NewF =
      Function::Create(FDecode->getFunctionType(), GlobalValue::PrivateLinkage,
                       "__omvll_decode", M);

  ValueToValueMapTy VMap;
  auto NewFArgsIt = NewF->arg_begin();
  auto FArgsIt    = FDecode->arg_begin();
  for (auto FArgsEnd = FDecode->arg_end(); FArgsIt != FArgsEnd; ++NewFArgsIt, ++FArgsIt) {
    VMap[&*FArgsIt] = &*NewFArgsIt;
  }
  SmallVector<ReturnInst*, 8> Returns;
  CloneFunctionInto(NewF, FDecode, VMap, CloneFunctionChangeType::DifferentModule, Returns);
  NewF->setDSOLocal(true);

  std::vector<Value*> Args = {
    DataPtr, DataPtr, KeyVal, VStrSize
  };

  if (NewF->arg_size() != 4) {
    fatalError(fmt::format("Expecting 4 arguments ({} provided)", NewF->arg_size()));
  }

  for (size_t i = 0; i < NewF->arg_size(); ++i) {
    auto* E = NewF->getFunctionType()->getParamType(i);
    auto* V = Args[i]->getType();
    if (E != V) {
      std::string err = fmt::format("Args #{}: Expecting {} while it is {}",
                                    i, ToString(*E), ToString(*V));
      fatalError(err.c_str());
    }
  }

  CallInst* callee = IRB.CreateCall(NewF->getFunctionType(), NewF, Args);
  IRB.CreateRetVoid();

  ctor_.push_back(F);
  inline_wlist_.push_back(callee);

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
    EI.HM = ExitOnErr(generateModule(Routine, HostTriple, "cpp",
                                     HOSTJIT->getContext(), {"-std=c++17"}));
  }

  {
    Ctx.setDiscardValueNames(false);
    EI.TM = ExitOnErr(
        generateModule(Routine, TargetTriple, "cpp", Ctx,
                       {"-target", TargetTriple.getTriple(), "-std=c++17"}));
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
  StringRef str = data.getRawDataValues();
  std::uniform_int_distribution<key_int_t> Dist(1, std::numeric_limits<key_int_t>::max());
  uint64_t key = Dist(*RNG_);
  //const auto* raw_key = reinterpret_cast<const uint8_t*>(&key);
  SDEBUG("Key for {}: 0x{:010x}", str.str(), key);

  std::vector<uint8_t> encoded(str.size());
  EncodingInfo EI(EncodingTy::STACK_LOOP);
  EI.key = key;

  genRoutines(Triple(BB.getModule()->getTargetTriple()), EI, BB.getContext());

  auto JIT = StringEncoding::HOSTJIT->compile(*EI.HM);
  if (auto E = JIT->lookup("encode")) {
    auto enc = reinterpret_cast<enc_routine_t>(E->getAddress());
    enc(encoded.data(), str.data(), key, str.size());
  } else {
    fatalError("Can't find 'encode' in the routine");
  }
  Constant* StrEnc = ConstantDataArray::get(BB.getContext(), encoded);
  G.setInitializer(StrEnc);

  auto it = gve_info_.insert({&G, std::move(EI)}).first;
  return injectOnStackLoop(BB, I, Op, G, *dyn_cast<ConstantDataSequential>(StrEnc), it->getSecond());
}

bool StringEncoding::processOnStack(BasicBlock& BB, Instruction& I, Use& Op, GlobalVariable& G,
                                    ConstantDataSequential& data, const StringEncOptStack& stack)
{

  std::uniform_int_distribution<uint8_t> Dist(1, 254);
  StringRef str = data.getRawDataValues();

  SDEBUG("[{}] {}: {}", name(), BB.getParent()->getName(),
                        data.isCString() ? data.getAsCString() : "<encoded>");
  if (str.size() >= stack.loopThreshold) {
    return processOnStackLoop(BB, I, Op, G, data);
  }
  std::vector<uint8_t> encoded(str.size());
  std::vector<uint8_t> key(str.size());
  std::generate(std::begin(key), std::end(key),
                [&Dist, this] () { return Dist(*RNG_); });

  for (size_t i = 0; i < str.size(); ++i) {
    encoded[i] = static_cast<uint8_t>(str[i]) ^ static_cast<uint8_t>(key[i]);
  }
  Constant* StrEnc = ConstantDataArray::get(BB.getContext(), encoded);
  G.setInitializer(StrEnc);

  EncodingInfo EI(EncodingTy::STACK);
  EI.key = std::move(key);
  auto it = gve_info_.insert({&G, std::move(EI)}).first;
  return injectOnStack(BB, I, Op, G, *dyn_cast<ConstantDataSequential>(StrEnc), it->getSecond());
}


}
