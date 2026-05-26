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

    switch (RandomGenerator::generateFullRand() % 2) {
    case 0:
      // (X | Y) - (X & Y)
      SDEBUG("[{}][Case 0] XOR choosing option (X | Y) - (X & Y)",
            Arithmetic::name());
      return BinaryOperator::CreateSub(Builder.CreateOr(X, Y),
                                       Builder.CreateAnd(X, Y), "mba_xor1");

    case 1:
      // (X - Y) + (2 * (~X & Y))
      SDEBUG("[{}][Case 1] XOR choosing option (X - Y) + (2 * (~X & Y))",
            Arithmetic::name());
      Value *InnerAnd = Builder.CreateAnd(Builder.CreateNot(X), Y);
      return BinaryOperator::CreateAdd(
          Builder.CreateSub(X, Y),
          Builder.CreateMul(InnerAnd, ConstantInt::get(X->getType(), 2)),
          "mba_xor2");
    }

    return nullptr;
  }

  Instruction *visitAdd(BinaryOperator &I) {
    Value *X, *Y;

    // Match X + Y
    if (!match(&I, m_Add(m_Value(X), m_Value(Y))))
      return nullptr;

    switch (RandomGenerator::generateFullRand() % 2) {
    case 0:
      // (X & Y) + (X | Y)
      SDEBUG("[{}][Case 0] ADD choosing option (X & Y) + (X | Y)",
            Arithmetic::name());
      return BinaryOperator::CreateAdd(Builder.CreateAnd(X, Y),
                                       Builder.CreateOr(X, Y), "mba_add1");

    case 1:
      // (X - ~Y) - 1
      SDEBUG("[{}][Case 1] ADD choosing option (X - ~Y) - 1",
            Arithmetic::name());
      Value *InnerSub = Builder.CreateSub(X, Builder.CreateNot(Y));
      return BinaryOperator::CreateSub(
          InnerSub, ConstantInt::get(X->getType(), 1), "mba_add2");
    }

    return nullptr;
  }

  Instruction *visitAnd(BinaryOperator &I) {
    Value *X, *Y;

    // Match X & Y
    if (!match(&I, m_And(m_Value(X), m_Value(Y))))
      return nullptr;

    switch (RandomGenerator::generateFullRand() % 2) {
    case 0:
      // (X + Y) - (X | Y)
      SDEBUG("[{}][Case 0] AND choosing option (X + Y) - (X | Y)",
            Arithmetic::name());
      return BinaryOperator::CreateSub(Builder.CreateAdd(X, Y),
                                       Builder.CreateOr(X, Y), "mba_and1");

    case 1:
      // (~X | Y) - ~X
      SDEBUG("[{}][Case 1] AND choosing option (~X | Y) - ~X",
            Arithmetic::name());
      Value *InnerOr = Builder.CreateOr(Builder.CreateNot(X), Y);
      return BinaryOperator::CreateSub(InnerOr, Builder.CreateNot(X),
                                       "mba_and2");
    }

    return nullptr;
  }

  Instruction *visitOr(BinaryOperator &I) {
    Value *X, *Y;

    // Match X | Y
    if (!match(&I, m_Or(m_Value(X), m_Value(Y))))
      return nullptr;

    switch (RandomGenerator::generateFullRand() % 3) {
    case 0:
      // X + Y + 1 + (~X | ~Y)
      SDEBUG("[{}][Case 0] OR choosing option X + Y + 1 + (~X | ~Y)",
            Arithmetic::name());
      return BinaryOperator::CreateAdd(
          Builder.CreateAdd(Builder.CreateAdd(X, Y),
                            ConstantInt::get(X->getType(), 1)),
          Builder.CreateOr(Builder.CreateNot(X), Builder.CreateNot(Y)),
          "mba_or1");

    case 1:
      // (X & ~Y) + Y
      SDEBUG("[{}][Case 1] OR choosing option (X & ~Y) + Y", Arithmetic::name());
      return BinaryOperator::CreateAdd(
          Builder.CreateAnd(X, Builder.CreateNot(Y)), Y, "mba_or2");

    case 2:
      // (X ^ Y) + (X & Y)
      SDEBUG("[{}][Case 2] OR choosing option (X ^ Y) + (X & Y)",
            Arithmetic::name());
      return BinaryOperator::CreateAdd(Builder.CreateXor(X, Y),
                                       Builder.CreateAnd(X, Y), "mba_or3");
    }

    return nullptr;
  }

  Instruction *visitSub(BinaryOperator &I) {
    Value *X, *Y;

    // Match X - Y
    if (!match(&I, m_Sub(m_Value(X), m_Value(Y))))
      return nullptr;

    switch (RandomGenerator::generateFullRand() % 2) {
    case 0:
      // (X ^ -Y) + (2 * (X & -Y))
      SDEBUG("[{}][Case 0] SUB choosing option (X ^ -Y) + (2 * (X & -Y))",
            Arithmetic::name());
      return BinaryOperator::CreateAdd(
          Builder.CreateXor(X, Builder.CreateNeg(Y)),
          Builder.CreateMul(Builder.CreateAnd(X, Builder.CreateNeg(Y)),
                            ConstantInt::get(X->getType(), 2)),
          "mba_sub1");

    case 1:
      // X + ~Y + 1
      SDEBUG("[{}][Case 1] SUB choosing option X + ~Y + 1", Arithmetic::name());
      return BinaryOperator::CreateAdd(
          Builder.CreateAdd(X, Builder.CreateNot(Y)),
          ConstantInt::get(X->getType(), 1), "mba_sub2");
    }

    return nullptr;
  }

  Instruction *visitInstruction(Instruction &) { return nullptr; }
};

bool Arithmetic::isSupported(const BinaryOperator &Op) {
  const auto Opcode = Op.getOpcode();
  return Opcode == Instruction::Add || Opcode == Instruction::Sub ||
         Opcode == Instruction::And || Opcode == Instruction::Xor ||
         Opcode == Instruction::Or;
}

bool Arithmetic::runOnBasicBlock(BasicBlock &BB,
                                 std::optional<size_t> ExplicitRounds) {
  bool Changed = false;
  
  ArithmeticVisitor::BuilderTy Builder(&BB);
  ArithmeticVisitor Visitor(Builder);

  // First pass: collect original candidates and their round counts
  SmallVector<std::pair<Instruction *, size_t>> Candidates;
  for (Instruction &I : BB) {
    size_t Rounds = 0;

    if (ExplicitRounds.has_value()) {
      Rounds = *ExplicitRounds;
    } else {
      // Check for per-instruction O-MVLL metadata first, then fall back to the
      // function-level opt.
      if (auto MO = getObf(I, MetaObfTy::OpaqueOp)) {
        if (auto *Val = MO->get<uint64_t>()) {
          Rounds = *Val;

          if (auto It = Opts.find(BB.getParent()); It != Opts.end()) {
            SINFO("[{}][{}] Metadata ({}) overrides rounds ({}) for {}", name(),
                  BB.getParent()->getName(), Rounds, It->second.Iterations,
                  I.getOpcodeName());
          }
        }
      } else if (auto It = Opts.find(BB.getParent()); It != Opts.end()) {
        Rounds = It->second.Iterations;
      }
    }

    if (Rounds == 0)
      continue;

    auto *BinOp = dyn_cast<BinaryOperator>(&I);
    if (!BinOp || !isSupported(*BinOp))
      continue;

    Candidates.emplace_back(&I, Rounds);
  }

  // All candidates in the same BB share the same round count.
  size_t MaxRounds = 0;
  for (auto &[_, R] : Candidates)
    MaxRounds = std::max(MaxRounds, R);

  // Iterate Rounds times over the full BB
  for (size_t Idx = 0; Idx < MaxRounds; ++Idx) {
    SmallVector<Instruction *> ToReplace;
    for (Instruction &I : BB) {
      if (getObf(I, MetaObfTy::OpaqueCst))
        continue;
      if (auto *BO = dyn_cast<BinaryOperator>(&I))
        if (isSupported(*BO))
          ToReplace.push_back(BO);
    }

    for (Instruction *Inst : ToReplace) {
      Builder.SetInsertPoint(Inst);
      Instruction *Result = Visitor.visit(*Inst);

      if (!Result || Result == Inst)
        continue;

      SDEBUG("[{}][{}] Replacing {} with {}", name(),
             BB.getParent()->getName(),
             Inst->getOpcodeName(), Result->getName());

      BasicBlock::iterator InsertPos = Inst->getIterator();
      Result->copyMetadata(
          *Inst, {LLVMContext::MD_dbg, LLVMContext::MD_annotation});
      Result->takeName(Inst);
      Result->insertInto(Inst->getParent(), InsertPos);
      Inst->replaceAllUsesWith(Result);
      Inst->eraseFromParent();

      Changed = true;
    }
  }

  return Changed;
}

bool Arithmetic::runOnFunction(Function &F,
                                 std::optional<size_t> ExplicitRounds){
  bool Changed = false;
  for (BasicBlock &BB : F)
    Changed |= runOnBasicBlock(BB, ExplicitRounds);
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

    Changed |= runOnFunction(*F);
  }

  SINFO("[{}] Changes {} applied on module {}", name(), Changed ? "" : "not",
        M.getName());

  ModuleChanges.notify(Changed);
  return ModuleChanges.report();
}

} // end namespace omvll
