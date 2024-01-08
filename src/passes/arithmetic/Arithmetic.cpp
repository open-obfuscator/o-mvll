#include "omvll/passes/arithmetic/Arithmetic.hpp"
#include "omvll/log.hpp"
#include "omvll/utils.hpp"
#include "omvll/PyConfig.hpp"
#include "omvll/ObfuscationConfig.hpp"
#include "omvll/passes/Metadata.hpp"

#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/IR/NoFolder.h"
#include "llvm/IR/PatternMatch.h"
#include "llvm/Support/RandomNumberGenerator.h"

using namespace llvm;
using namespace PatternMatch;

namespace omvll {

/* This flag can be used for disabling the inlining of the MBA function wrappers
 * (mostly for debugging)
 */
static constexpr bool INLINE_WRAPPER = true;

// LLVM's InstVisitor to pattern match and replace arithmetic operations with MBA
// The current MBA are take from sspam: https://github.com/quarkslab/sspam/blob/master/sspam/simplifier.py#L30-L53
struct ArithmeticVisitor : public InstVisitor<ArithmeticVisitor, Instruction*> {
  using BuilderTy = IRBuilder<NoFolder>;
  BuilderTy &Builder;

  ArithmeticVisitor(BuilderTy& B):
    Builder(B) {}

  Instruction* visitXor(BinaryOperator &I) {
    Value *X, *Y;

    // Match X ^ Y
    if (!match(&I, m_Xor(m_Value(X), m_Value(Y)))) {
      return nullptr;
    }

    // (X | Y) - (X & Y)
    return BinaryOperator::CreateSub(
        Builder.CreateOr(X, Y),
        Builder.CreateAnd(X, Y),
        "mba_xor"
    );
  }

  Instruction* visitAdd(BinaryOperator &I) {
    Value *X, *Y;

    // Match X + Y
    if (!match(&I, m_Add(m_Value(X), m_Value(Y)))) {
      return nullptr;
    }

    // (A & B) + (A | B)
    return BinaryOperator::CreateAdd(
        Builder.CreateAnd(X, Y),
        Builder.CreateOr(X, Y),
        "mba_add"
    );
  }

  Instruction* visitAnd(BinaryOperator &I) {
    Value *X, *Y;

    // Match X & Y
    if (!match(&I, m_And(m_Value(X), m_Value(Y)))) {
      return nullptr;
    }

    // (X + Y) - (X | Y)
    return BinaryOperator::CreateSub(
        Builder.CreateAdd(X, Y),
        Builder.CreateOr(X, Y),
        "mba_and"
    );
  }

  Instruction* visitOr(BinaryOperator &I) {
    Value *X, *Y;

    // Match X | Y
    if (!match(&I, m_Or(m_Value(X), m_Value(Y)))) {
      return nullptr;
    }

    // X + Y + 1 + (~X | ~Y)
    return BinaryOperator::CreateAdd(
        Builder.CreateAdd(
          Builder.CreateAdd(X, Y),
          ConstantInt::get(X->getType(), 1)
        ),
        Builder.CreateOr(
          Builder.CreateNot(X),
          Builder.CreateNot(Y)
        ), "mba_or"
    );
  }

  Instruction* visitSub(BinaryOperator &I) {
    Value *X, *Y;

    // Match X - Y
    if (!match(&I, m_Sub(m_Value(X), m_Value(Y)))) {
      return nullptr;
    }

    // (X ^ -Y) + 2*(X & -Y)
    return BinaryOperator::CreateAdd(
        Builder.CreateXor(X, Builder.CreateNeg(Y)),
        Builder.CreateMul(ConstantInt::get(X->getType(), 2),
          Builder.CreateAnd(X, Builder.CreateNeg(Y))
        ), "mba_sub"
    );
  }

  Instruction* visitInstruction(Instruction&) { return nullptr; }
};

bool Arithmetic::isSupported(const BinaryOperator& Op) {
  const auto opcode = Op.getOpcode();
  return opcode == Instruction::Add ||
         opcode == Instruction::Sub ||
         opcode == Instruction::And ||
         opcode == Instruction::Xor ||
         opcode == Instruction::Or;
}

Function* Arithmetic::injectFunWrapper(Module& M, BinaryOperator& Op, Value& Lhs, Value& Rhs) {
  auto* FTy = FunctionType::get(Op.getType(), {Lhs.getType(), Rhs.getType()}, /*vargs=*/false);
  auto* F   = Function::Create(FTy, llvm::GlobalValue::PrivateLinkage,
                               "__omvll_mba", M);
  if constexpr (INLINE_WRAPPER) {
    F->addFnAttr(Attribute::AlwaysInline);
  } else {
    F->addFnAttr(Attribute::NoInline);
  }

  F->addFnAttr(Attribute::OptimizeNone);
  F->setCallingConv(CallingConv::C);

  BasicBlock* EntryBB = BasicBlock::Create(F->getContext(), "entry", F);
  Value* p1 = F->getArg(0);
  Value* p2 = F->getArg(1);
  IRBuilder<> EBuild(EntryBB);
  EBuild.SetInsertPoint(EntryBB);
  EBuild.CreateRet(EBuild.CreateBinOp(Op.getOpcode(), p1, p2));
  return F;
}

bool Arithmetic::runOnBasicBlock(BasicBlock &BB) {
  size_t counter = 0;

  SmallVector<Instruction *> ToErase;
  DenseMap<Function*, size_t> ToObfuscate;
  for (Instruction& I : BB) {
    // First, check if there is O-MVLL metadata associated with the current instruction.
    // If it is the case, access the number of iterations
    size_t nbRounds = 0;
    if (auto MO = getObf(I, MObfTy::OPAQUE_OP)) {
      if (auto* Val = MO->get<uint64_t>()) {
        nbRounds = *Val;
      }
    }
    else if (auto it = opts_.find(BB.getParent()); it != opts_.end()) {
      nbRounds = it->second.iterations;
    }

    // No need to continue if there is a 0-round
    if (nbRounds == 0) {
      continue;
    }

    // Now, check that we are on a BinOp instruction
    auto* BinOp = dyn_cast<BinaryOperator>(&I);
    if (BinOp == nullptr) {
      continue;
    }

    if (!isSupported(*BinOp)) {
      continue;
    }

    Value* lhs = BinOp->getOperand(0);
    Value* rhs = BinOp->getOperand(1);

    Function* FWrap = injectFunWrapper(*BB.getModule(), *BinOp, *lhs, *rhs);
    ToObfuscate[FWrap] = nbRounds;

    IRBuilder<> IRB(&I);
    Instruction* Result = IRB.CreateCall(FWrap->getFunctionType(), FWrap, {lhs, rhs});

    ++counter;

    I.replaceAllUsesWith(Result);
    ToErase.emplace_back(&I);
  }

  for_each(ToErase, [](Instruction *I) { I->eraseFromParent(); });
  ToErase.clear();

  ArithmeticVisitor::BuilderTy Builder(&BB);
  ArithmeticVisitor Visitor(Builder);

  for (auto& [F, round] : ToObfuscate) {
    for (BasicBlock& BB : *F) {
      for (size_t i = 0; i < round; ++i) {
        for (Instruction& I : BB) {
          Builder.SetInsertPoint(&I);
          Instruction *Result = Visitor.visit(I);

          if (Result == nullptr || Result == &I) {
            continue;
          }

          BasicBlock *InstParent = I.getParent();
          BasicBlock::iterator InsertPos = I.getIterator();

          Result->copyMetadata(I, {LLVMContext::MD_dbg, LLVMContext::MD_annotation});
          Result->takeName(&I);
          InstParent->getInstList().insert(InsertPos, Result);
          I.replaceAllUsesWith(Result);
          //ToErase.emplace_back(&I);
        }
      }
    }
  }

  //for_each(ToErase, [](Instruction *I) { I->eraseFromParent(); });
  bool Changed = counter > 0;
  return Changed;
}

PreservedAnalyses Arithmetic::run(Module &M,
                                  ModuleAnalysisManager &FAM) {
  RNG_ = M.createRNG(name());
  SDEBUG("Running {} on {}", name(), M.getName().str());
  bool Changed = false;

  PyConfig& config = PyConfig::instance();

  auto& Fs = M.getFunctionList();

  // "Backup" all the functions since the pass adds new functions and thus,
  // break the iterator
  std::vector<Function*> LFs;
  LFs.reserve(Fs.size());
  std::transform(Fs.begin(), Fs.end(), std::back_inserter(LFs),
                 [] (Function& F) { return &F; });


  for (Function* F : LFs) {
    ArithmeticOpt opt = config.getUserConfig()->obfuscate_arithmetic(&M, F);
    if (!opt)
      continue;

    opts_.insert({F, std::move(opt)});

    for (BasicBlock& BB : *F) {
      Changed |= runOnBasicBlock(BB);
    }
  }
  SINFO("[{}] Done!", name());
  return Changed ? PreservedAnalyses::none() :
                   PreservedAnalyses::all();

}
}

