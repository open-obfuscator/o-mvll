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

    switch (RandomGenerator::generateFullRand() % 2) {
    case 0:
      // (X | Y) - (X & Y)
      SINFO("[{}] XOR choosing option (X | Y) - (X & Y)", Arithmetic::name());
      return BinaryOperator::CreateSub(Builder.CreateOr(X, Y),
                                       Builder.CreateAnd(X, Y), "mba_xor1");

    case 1:
      // (X - Y) + (2 * (~X & Y))
      SINFO("[{}] XOR choosing option (X - Y) + (2 * (~X & Y))",
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
      SINFO("[{}] ADD choosing option (X & Y) + (X | Y)", Arithmetic::name());
      return BinaryOperator::CreateAdd(Builder.CreateAnd(X, Y),
                                       Builder.CreateOr(X, Y), "mba_add1");

    case 1:
      // (X - ~Y) - 1
      SINFO("[{}] ADD choosing option (X - ~Y) - 1", Arithmetic::name());
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
      SINFO("[{}] AND choosing option (X + Y) - (X | Y)", Arithmetic::name());
      return BinaryOperator::CreateSub(Builder.CreateAdd(X, Y),
                                       Builder.CreateOr(X, Y), "mba_and1");

    case 1:
      // (~X | Y) - ~X
      SINFO("[{}] AND choosing option (~X | Y) - ~X", Arithmetic::name());
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
      SINFO("[{}] OR choosing option X + Y + 1 + (~X | ~Y)",
            Arithmetic::name());
      return BinaryOperator::CreateAdd(
          Builder.CreateAdd(Builder.CreateAdd(X, Y),
                            ConstantInt::get(X->getType(), 1)),
          Builder.CreateOr(Builder.CreateNot(X), Builder.CreateNot(Y)),
          "mba_or1");

    case 1:
      // (X & ~Y) + Y
      SINFO("[{}] OR choosing option (X & ~Y) + Y", Arithmetic::name());
      return BinaryOperator::CreateAdd(
          Builder.CreateAnd(X, Builder.CreateNot(Y)), Y, "mba_or2");

    case 2:
      // (X ^ Y) + (X & Y)
      SINFO("[{}] OR choosing option (X ^ Y) + (X & Y)", Arithmetic::name());
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
      SINFO("[{}] SUB choosing option (X ^ -Y) + (2 * (X & -Y))",
            Arithmetic::name());
      return BinaryOperator::CreateAdd(
          Builder.CreateXor(X, Builder.CreateNeg(Y)),
          Builder.CreateMul(Builder.CreateAnd(X, Builder.CreateNeg(Y)),
                            ConstantInt::get(X->getType(), 2)),
          "mba_sub1");

    case 1:
      // X + ~Y + 1
      SINFO("[{}] SUB choosing option X + ~Y + 1", Arithmetic::name());
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
  MapVector<Function *, size_t> ToObfuscate;
  for (Instruction &I : BB) {
    // First, check if there is O-MVLL metadata associated with the current
    // instruction. If it is the case, access the number of iterations.
    size_t Rounds = 0;
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
          if (getObf(I, MetaObfTy::OpaqueCst))
            continue;

          Builder.SetInsertPoint(&I);
          Instruction *Result = Visitor.visit(I);

          if (!Result || Result == &I)
            continue;

          SINFO("[{}][{}] Replacing {} with {}", name(), F->getName(),
                I.getOpcodeName(), Result->getName());

          BasicBlock *InstParent = I.getParent();
          BasicBlock::iterator InsertPos = I.getIterator();

          Result->copyMetadata(
              I, {LLVMContext::MD_dbg, LLVMContext::MD_annotation});
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
