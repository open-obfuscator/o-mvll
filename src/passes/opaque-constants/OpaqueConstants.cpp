
#include "omvll/ObfuscationConfig.hpp"
#include "omvll/PyConfig.hpp"
#include "omvll/log.hpp"
#include "omvll/passes/Metadata.hpp"
#include "omvll/passes/opaque-constants/OpaqueConstants.hpp"
#include "omvll/utils.hpp"
#include "omvll/visitvariant.hpp"

#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/NoFolder.h"
#include "llvm/Support/RandomNumberGenerator.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "GenOpaque.hpp"

using namespace llvm;

namespace omvll {

inline bool isEligible(const Instruction& I) {
  return !isa<CallInst>(I) &&
         !isa<GetElementPtrInst>(I) &&
         !isa<SwitchInst>(I);
}

inline bool isSkip(const OpaqueConstantsOpt& opt) {
  return std::get_if<OpaqueConstantsSkip>(&opt) != nullptr;
}

bool OpaqueConstants::Process(Instruction& I, Use& Op, ConstantInt& CI, OpaqueConstantsOpt* opt) {
  if (!isEligible(I)) {
    return false;
  }
  BasicBlock& BB = *I.getParent();

  OpaqueContext* ctx = getOrCreateContext(BB);
  if (ctx == nullptr) {
    SWARN("Can't opaque {}", ToString(BB));
    return false;
  }
  /*
   * Special processing for 0 values
   * ===============================
   */
  if (CI.isZero()) {
    if (opt != nullptr) {
      bool shouldProtect = std::visit(overloaded {
        [] (OpaqueConstantsSkip&)         { return false;         },
        [] (OpaqueConstantsBool& v)       { return v.value;       },
        [] (OpaqueConstantsLowerLimit& v) { return false;         },
        [] (OpaqueConstantsSet& v)        { return v.contains(0); },
      }, *opt);

      if (!shouldProtect) {
        return false;
      }
    }

    Value* NewZero = GetOpaqueZero(I, *ctx, CI.getType());
    if (NewZero == nullptr) {
      SWARN("Can't opaque {}", ToString(CI));
      return false;
    }
    Op.set(NewZero);
    return true;
  }

  /*
   * Special processing for 1 values
   * ===============================
   */
  if (CI.isOne()) {
    if (opt != nullptr) {
      bool shouldProtect = std::visit(overloaded {
        [] (OpaqueConstantsSkip&)         { return false;         },
        [] (OpaqueConstantsBool& v)       { return v.value;       },
        [] (OpaqueConstantsLowerLimit& v) { return v.value >= 1;  },
        [] (OpaqueConstantsSet& v)        { return v.contains(1); },
      }, *opt);

      if (!shouldProtect) {
        return false;
      }
    }

    Value* NewOne = GetOpaqueOne(I, *ctx, CI.getType());
    if (NewOne == nullptr) {
      SWARN("Can't opaque {}", ToString(CI));
      return false;
    }
    Op.set(NewOne);
    return true;
  }

  bool shouldProtect = std::visit(overloaded {
    []    (OpaqueConstantsSkip&)         { return false;   },
    []    (OpaqueConstantsBool& v)       { return v.value; },
    [&CI] (OpaqueConstantsLowerLimit& v) { return CI.getLimitedValue() > v.value;   },
    [&CI] (OpaqueConstantsSet& v)        {
      static constexpr uint64_t MAGIC = 0x4208d8df2c6415bc;
      const uint64_t LV = CI.getLimitedValue(MAGIC);
      if (LV == MAGIC) {
        return true;
      }
      return !v.empty() && v.contains(LV);
    },
  }, *opt);

  if (!shouldProtect) {
    return false;
  }

  Value* NewCst = GetOpaqueCst(I, *ctx, CI);

  if (NewCst == nullptr) {
    SWARN("Can't opaque {}", ToString(CI));
    return false;
  }
  Op.set(NewCst);
  return true;
}

Value* OpaqueConstants::GetOpaqueZero(Instruction& I, OpaqueContext& C, Type* Ty) {
  static constexpr auto MAX_CASES = 3;
  static_assert(RandomNumberGenerator::max() >= MAX_CASES);
  std::uniform_int_distribution<uint8_t> Dist(1, MAX_CASES);

  uint8_t Sel = Dist(*RNG_);
  switch (Sel) {
    case 1: return GetOpaqueZero_1(I, C, Ty, *RNG_);
    case 2: return GetOpaqueZero_2(I, C, Ty, *RNG_);
    case 3: return GetOpaqueZero_3(I, C, Ty, *RNG_);
    default:
      {
        SWARN("The RNG number ({}) is out of range for generating opaque zero", Sel);
        return nullptr;
      }
  }
}

Value* OpaqueConstants::GetOpaqueOne(Instruction& I, OpaqueContext& C, Type* Ty) {
  static constexpr auto MAX_CASES = 3;
  static_assert(RandomNumberGenerator::max() >= MAX_CASES);
  std::uniform_int_distribution<uint8_t> Dist(1, MAX_CASES);

  uint8_t Sel = Dist(*RNG_);
  switch (Sel) {
    case 1: return GetOpaqueOne_1(I, C, Ty, *RNG_);
    case 2: return GetOpaqueOne_2(I, C, Ty, *RNG_);
    case 3: return GetOpaqueOne_3(I, C, Ty, *RNG_);
    default:
      {
        SWARN("The RNG number ({}) is out of range for generating opaque one", Sel);
        return nullptr;
      }
  }
}


Value* OpaqueConstants::GetOpaqueCst(Instruction& I, OpaqueContext& C, const ConstantInt& CI) {
  static constexpr auto MAX_CASES = 3;
  static_assert(RandomNumberGenerator::max() >= MAX_CASES);
  std::uniform_int_distribution<uint8_t> Dist(1, MAX_CASES);

  uint8_t Sel = Dist(*RNG_);
  switch (Sel) {
    case 1: return GetOpaqueConst_1(I, C, CI, *RNG_);
    case 2: return GetOpaqueConst_2(I, C, CI, *RNG_);
    case 3: return GetOpaqueConst_3(I, C, CI, *RNG_);
    default:
      {
        SWARN("The RNG number ({}) is out of range for generating opaque value", Sel);
        return nullptr;
      }
  }
}

bool OpaqueConstants::Process(Instruction& I, OpaqueConstantsOpt* opt) {
  bool Changed = false;
  #ifdef OMVLL_DEBUG
    std::string InstStr = ToString(I);
  #endif
  for (Use& Op : I.operands()) {
    if (auto* CI = dyn_cast<ConstantInt>(Op)) {
      Changed |= Process(I, Op, *CI, opt);
    }
  }

  #ifdef OMVLL_DEBUG
  if (Changed) {
    SDEBUG("--> OPAQUE CST: {}", InstStr);
  }
  #endif
  return Changed;
}


OpaqueContext* OpaqueConstants::getOrCreateContext(BasicBlock& BB) {
  if (auto it = ctx_.find(&BB); it != ctx_.end()) {
    return &it->second;
  }

  auto IP = BB.getFirstInsertionPt();
  if (IP == BB.end()) {
    return nullptr;
  }
  OpaqueContext& ctx = ctx_[&BB];
  IRBuilder<NoFolder> IRB(&*IP);
  ctx.T1 = IRB.CreateAlloca(IRB.getInt64Ty());
  ctx.T2 = IRB.CreateAlloca(IRB.getInt64Ty());

  IRB.CreateAlignedStore(
    IRB.CreatePtrToInt(ctx.T2, IRB.getInt64Ty()), ctx.T1, Align(8));

  IRB.CreateAlignedStore(
    IRB.CreatePtrToInt(ctx.T1, IRB.getInt64Ty()), ctx.T2, Align(8));

  return &ctx;
}

bool OpaqueConstants::runOnBasicBlock(llvm::BasicBlock &BB, OpaqueConstantsOpt* opt) {
  bool Changed = false;

  for (Instruction& I : BB) {
    if (hasObf(I, MObfTy::OPAQUE_CST)) {
      OpaqueConstantsOpt force = OpaqueConstantsBool(true);
      Changed |= Process(I, &force);
    }
    else if (opt != nullptr) {
      Changed |= Process(I, opt);
    }
  }
  return Changed;
}

PreservedAnalyses OpaqueConstants::run(Module &M,
                                       ModuleAnalysisManager &FAM) {
  RNG_ = M.createRNG(name());

  bool Changed = false;
  auto* Int64Ty = Type::getInt64Ty(M.getContext());
  M.getOrInsertGlobal("__omvll_opaque_gv", Int64Ty,
      [&] () {
        return new GlobalVariable(M, Int64Ty, /*isConstant=*/false,
                                  GlobalValue::PrivateLinkage,
                                  ConstantInt::get(Int64Ty, 0),
                                  "__omvll_opaque_gv");
      });


  PyConfig& config = PyConfig::instance();

  for (Function& F : M) {
    OpaqueConstantsOpt opt = config.getUserConfig()->obfuscate_constants(&M, &F);
    OpaqueConstantsOpt* inserted = nullptr;
    if (isSkip(opt))
      continue;

    auto ret = opts_.insert({&F, std::move(opt)});
    if (ret.second) {
      inserted = &ret.first->second;
    }

    for (BasicBlock& BB : F) {
      Changed |= runOnBasicBlock(BB, inserted);
    }
  }
  SINFO("[{}] Done!", name());
  return Changed ? PreservedAnalyses::none() :
                   PreservedAnalyses::all();

}
}

