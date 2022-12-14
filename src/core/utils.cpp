#include "omvll/utils.hpp"
#include "omvll/log.hpp"

#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/MemoryBuffer.h>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/RandomNumberGenerator.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/Local.h>

using namespace llvm;

namespace omvll {

std::string ToString(const Module& M) {
  std::error_code ec;
  std::string out;
  raw_string_ostream os(out);
  M.print(os, nullptr);
  return out;
}

std::string ToString(const Instruction& I) {
  std::string out;
  raw_string_ostream(out) << I;
  return out;
}

std::string ToString(const BasicBlock& BB) {
  std::string out;
  raw_string_ostream os(out);
  BB.printAsOperand(os, true);
  return out;
}

std::string ToString(const Type& Ty) {
  std::string out;
  raw_string_ostream os(out);
  os << TypeIDStr(Ty) << ": " << Ty;
  return out;
}

std::string ToString(const Value& V) {
  std::string out;
  raw_string_ostream os(out);
  os << ValueIDStr(V) << ": " << V;
  return out;
}


std::string ToString(const MDNode& N) {
  std::string out;
  raw_string_ostream os(out);
  N.printTree(os);
  return out;
}

std::string TypeIDStr(const Type& Ty) {
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
  }
}

std::string ValueIDStr(const Value& V) {


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

void dump(Module& M, const std::string& file) {
  std::error_code ec;
  raw_fd_ostream fd(file, ec);
  M.print(fd, nullptr);
}

void dump(Function& F, const std::string& file) {
  std::error_code ec;
  raw_fd_ostream fd(file, ec);
  F.print(fd, nullptr);
}

void dump(const MemoryBuffer& MB, const std::string& file) {
  std::error_code ec;
  raw_fd_ostream fd(file, ec);
  fd << MB.getBuffer();
}

size_t demotePHINode(Function& F) {
  size_t count = 0;
  std::vector<PHINode*> phiNodes;
  do {
    phiNodes.clear();
    for (auto& BB : F) {
      for (auto& I : BB.phis()) {
        phiNodes.push_back(&I);
      }
    }
    count += phiNodes.size();
    for (PHINode* phi : phiNodes) {
      DemotePHIToStack(phi, F.begin()->getTerminator());
    }
  } while (!phiNodes.empty());
  return count;
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

size_t demoteRegs(Function& F) {
  size_t count = 0;
  std::list<Instruction*> WorkList;
  BasicBlock* BBEntry = &F.getEntryBlock();
  do {
    WorkList.clear();
    for (BasicBlock& BB : F) {
      for (Instruction& I : BB) {
        if (!(isa<AllocaInst>(I) && I.getParent() == BBEntry) && valueEscapes(I)) {
          WorkList.push_front(&I);
        }
      }
    }
    count += WorkList.size();
    for (Instruction* I : WorkList) {
      DemoteRegToStack(*I, false, F.begin()->getTerminator());
    }
  } while (!WorkList.empty());
  return count;
}

size_t reg2mem(Function& F) {
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
  size_t count = 0;
  // Insert all new allocas into entry block.
  BasicBlock *BBEntry = &F.getEntryBlock();
  assert(pred_empty(BBEntry) &&
         "Entry block to function must not have predecessors!");

  // Find first non-alloca instruction and create insertion point. This is
  // safe if block is well-formed: it always have terminator, otherwise
  // we'll get and assertion.
  BasicBlock::iterator I = BBEntry->begin();
  while (isa<AllocaInst>(I)) ++I;

  CastInst *AllocaInsertionPoint = new BitCastInst(
      Constant::getNullValue(Type::getInt32Ty(F.getContext())),
      Type::getInt32Ty(F.getContext()), "reg2mem alloca point", &*I);

  // Find the escaped instructions. But don't create stack slots for
  // allocas in entry block.
  std::list<Instruction*> WorkList;
  for (Instruction &I : instructions(F))
    if (!(isa<AllocaInst>(I) && I.getParent() == BBEntry) && valueEscapes(I))
      WorkList.push_front(&I);

  // Demote escaped instructions
  count += WorkList.size();
  for (Instruction *I : WorkList)
    DemoteRegToStack(*I, false, AllocaInsertionPoint);

  WorkList.clear();

  // Find all phi's
  for (BasicBlock &BB : F)
    for (auto &Phi : BB.phis())
      WorkList.push_front(&Phi);

  // Demote phi nodes
  count += WorkList.size();
  for (Instruction *I : WorkList)
    DemotePHIToStack(cast<PHINode>(I), AllocaInsertionPoint);

  return count;
}

void shuffleFunctions(Module& M) {
  /*
   * The iterator associated getFunctionList() is not "random"
   * so we can't std::shuffle() the list.
   *
   * On the other hand, getFunctionList() has a sort method that we can
   * use to (randomly?) shuffle in list in place.
   */
  DenseMap<Function*, uint32_t> Values;
  DenseSet<uint64_t> Taken;

  std::mt19937_64 gen64;
  std::uniform_int_distribution<uint32_t> Dist(0, std::numeric_limits<uint32_t>::max() - 1);

  for (Function& F : M) {
    uint64_t ID = Dist(gen64);
    while (!Taken.insert(ID).second) {
      ID = Dist(gen64);
    }
    Values[&F] = ID;
  }

  auto& List = M.getFunctionList();
  List.sort([&Values] (Function& LHS, Function& RHS) {
              return Values[&LHS] < Values[&RHS];
            });
}

void fatalError(const std::string& msg) {
  fatalError(msg.c_str());
}

void fatalError(const char* msg) {
  static LLVMContext Ctx;
  Ctx.emitError(msg);

  // emitError could return, so we make sure that we stop the execution
  SERR("Error: {}", msg);
  std::abort();
}

}
