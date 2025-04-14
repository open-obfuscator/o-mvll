//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "llvm/IR/Constants.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/NoFolder.h"
#include "llvm/IR/PatternMatch.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include "omvll/ObfuscationConfig.hpp"
#include "omvll/PyConfig.hpp"
#include "omvll/log.hpp"
#include "omvll/passes/Metadata.hpp"
#include "omvll/passes/arithmetic/Arithmetic.hpp"
#include "omvll/utils.hpp"

using namespace llvm;
using namespace PatternMatch;

static constexpr auto MBAFunctionName = "__omvll_mba";

// This flag can be used for disabling the inlining of the MBA function wrappers
// (mostly for debugging).
static constexpr bool ShouldInline = true;

namespace omvll {

// LLVM's InstVisitor to pattern match and replace arithmetic operations with
// MBA The current MBA are take from sspam:
// https://github.com/quarkslab/sspam/blob/master/sspam/simplifier.py#L30-L53
struct ArithmeticVisitor
    : public InstVisitor<ArithmeticVisitor, Instruction *> {
  using BuilderTy = IRBuilder<NoFolder>;
  BuilderTy &Builder;

  ArithmeticVisitor(BuilderTy &B) : Builder(B) {}

  Instruction *visitXor(BinaryOperator &I) {
    Value *X, *Y;

    // Match X ^ Y
    if (!match(&I, m_Xor(m_Value(X), m_Value(Y))))
      return nullptr;

    // (X | Y) - (X & Y)
    return BinaryOperator::CreateSub(Builder.CreateOr(X, Y),
                                     Builder.CreateAnd(X, Y), "mba_xor");
  }

  Instruction *visitAdd(BinaryOperator &I) {
    Value *X, *Y;

    // Match X + Y
    if (!match(&I, m_Add(m_Value(X), m_Value(Y))))
      return nullptr;

    // (A & B) + (A | B)
    return BinaryOperator::CreateAdd(Builder.CreateAnd(X, Y),
                                     Builder.CreateOr(X, Y), "mba_add");
  }

  Instruction *visitAnd(BinaryOperator &I) {
    Value *X, *Y;

    // Match X & Y
    if (!match(&I, m_And(m_Value(X), m_Value(Y))))
      return nullptr;

    // (X + Y) - (X | Y)
    return BinaryOperator::CreateSub(Builder.CreateAdd(X, Y),
                                     Builder.CreateOr(X, Y), "mba_and");
  }

  Instruction *visitOr(BinaryOperator &I) {
    Value *X, *Y;

    // Match X | Y
    if (!match(&I, m_Or(m_Value(X), m_Value(Y))))
      return nullptr;

    // X + Y + 1 + (~X | ~Y)
    return BinaryOperator::CreateAdd(
        Builder.CreateAdd(Builder.CreateAdd(X, Y),
                          ConstantInt::get(X->getType(), 1)),
        Builder.CreateOr(Builder.CreateNot(X), Builder.CreateNot(Y)), "mba_or");
  }

  Instruction *visitSub(BinaryOperator &I) {
    Value *X, *Y;

    // Match X - Y
    if (!match(&I, m_Sub(m_Value(X), m_Value(Y))))
      return nullptr;

    // (X ^ -Y) + 2*(X & -Y)
    return BinaryOperator::CreateAdd(
        Builder.CreateXor(X, Builder.CreateNeg(Y)),
        Builder.CreateMul(ConstantInt::get(X->getType(), 2),
                          Builder.CreateAnd(X, Builder.CreateNeg(Y))),
        "mba_sub");
  }

  Instruction *visitInstruction(Instruction &) { return nullptr; }
};

bool Arithmetic::isSupported(const BinaryOperator &Op) {
  const auto Opcode = Op.getOpcode();
  return Opcode == Instruction::Add || Opcode == Instruction::Sub ||
         Opcode == Instruction::And || Opcode == Instruction::Xor ||
         Opcode == Instruction::Or;
}

Function *Arithmetic::injectFunWrapper(Module &M, BinaryOperator &Op,
                                       Value &Lhs, Value &Rhs) {
  auto *FTy = FunctionType::get(Op.getType(), {Lhs.getType(), Rhs.getType()},
                                /*vargs=*/false);
  auto *F = Function::Create(FTy, llvm::GlobalValue::PrivateLinkage,
                             MBAFunctionName, M);

  if constexpr (ShouldInline) {
    F->addFnAttr(Attribute::AlwaysInline);
  } else {
    F->addFnAttr(Attribute::NoInline);
    F->addFnAttr(Attribute::OptimizeNone);
  }

  F->setCallingConv(CallingConv::C);

  BasicBlock *EntryBB = BasicBlock::Create(F->getContext(), "entry", F);
  Value *P1 = F->getArg(0);
  Value *P2 = F->getArg(1);
  IRBuilder<> EBuild(EntryBB);
  EBuild.SetInsertPoint(EntryBB);
  EBuild.CreateRet(EBuild.CreateBinOp(Op.getOpcode(), P1, P2));
  return F;
}

bool Arithmetic::runOnBasicBlock(BasicBlock &BB) {
  size_t Counter = 0;

  SmallVector<Instruction *> ToErase;
  DenseMap<Function *, size_t> ToObfuscate;
  for (Instruction &I : BB) {
    // First, check if there is O-MVLL metadata associated with the current
    // instruction. If it is the case, access the number of iterations.
    size_t Rounds = 0;
    if (auto MO = getObf(I, MetaObfTy::OpaqueOp)) {
      if (auto *Val = MO->get<uint64_t>()) {
        Rounds = *Val;
      }
    } else if (auto It = Opts.find(BB.getParent()); It != Opts.end()) {
      Rounds = It->second.Iterations;
    }

    // No need to continue if there is a 0-round.
    if (Rounds == 0)
      continue;

    // Now check that we are in a BinOp instruction.
    auto *BinOp = dyn_cast<BinaryOperator>(&I);
    if (!BinOp)
      continue;

    if (!isSupported(*BinOp))
      continue;

    Value *Lhs = BinOp->getOperand(0);
    Value *Rhs = BinOp->getOperand(1);

    Function *FWrap = injectFunWrapper(*BB.getModule(), *BinOp, *Lhs, *Rhs);
    ToObfuscate[FWrap] = Rounds;

    IRBuilder<> IRB(&I);
    Instruction *Result =
        IRB.CreateCall(FWrap->getFunctionType(), FWrap, {Lhs, Rhs});

    ++Counter;

    I.replaceAllUsesWith(Result);
    ToErase.emplace_back(&I);
  }

  for_each(ToErase, [](Instruction *I) { I->eraseFromParent(); });
  ToErase.clear();

  ArithmeticVisitor::BuilderTy Builder(&BB);
  ArithmeticVisitor Visitor(Builder);

  for (const auto &[F, Round] : ToObfuscate) {
    for (BasicBlock &BB : *F) {
      for (size_t Idx = 0; Idx < Round; ++Idx) {
        for (Instruction &I : BB) {
          Builder.SetInsertPoint(&I);
          Instruction *Result = Visitor.visit(I);

          if (!Result || Result == &I)
            continue;

          SINFO("[{}][{}] Replacing {} with {}", name(), F->getName(),
                I.getName(), Result->getName());

          BasicBlock *InstParent = I.getParent();
          BasicBlock::iterator InsertPos = I.getIterator();

          Result->copyMetadata(I, {LLVMContext::MD_dbg, LLVMContext::MD_annotation});
          Result->takeName(&I);
          Result->insertInto(InstParent, InsertPos);
          I.replaceAllUsesWith(Result);
        }
      }
    }
  }

  bool Changed = Counter > 0;
  return Changed;
}

PreservedAnalyses Arithmetic::run(Module &M, ModuleAnalysisManager &FAM) {
  if (isModuleGloballyExcluded(&M)) {
    SINFO("Excluding module [{}]", M.getName());
    return PreservedAnalyses::all();
  }

  bool Changed = false;
  PyConfig &Config = PyConfig::instance();
  SINFO("[{}] Executing on module {}", name(), M.getName());
  RNG = M.createRNG(name());
  IRChangesMonitor ModuleChanges(M, name());

  // Backup all the functions since the pass adds new functions and thus, break
  // the iterator.
  std::vector<Function *> ToVisit;
  auto &Functions = M.getFunctionList();
  ToVisit.reserve(Functions.size());
  std::transform(Functions.begin(), Functions.end(),
                 std::back_inserter(ToVisit), [](auto &F) { return &F; });

  for (Function *F : ToVisit) {
    if (isFunctionGloballyExcluded(F))
      continue;

    ArithmeticOpt Opt = Config.getUserConfig()->obfuscateArithmetics(&M, F);
    if (!Opt)
      continue;

    SINFO("[{}] Visiting function {}", name(), F->getName());
    Opts.insert({F, std::move(Opt)});

    for (BasicBlock &BB : *F)
      Changed |= runOnBasicBlock(BB);
  }

  SINFO("[{}] Changes {} applied on module {}", name(), Changed ? "" : "not",
        M.getName());

  ModuleChanges.notify(Changed);
  return ModuleChanges.report();
}

} // end namespace omvll
