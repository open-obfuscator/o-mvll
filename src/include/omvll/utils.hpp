#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include <string>

#include "llvm/ADT/StringRef.h"
#include "llvm/TargetParser/Triple.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/Error.h"

// Forward declarations
namespace llvm {
class Instruction;
class BasicBlock;
class Function;
class Module;
class Type;
class Value;
class MDNode;
class MemoryBuffer;
} // end namespace llvm

namespace omvll {

std::string ToString(const llvm::Module &M);
std::string ToString(const llvm::BasicBlock &BB);
std::string ToString(const llvm::Instruction &I);
std::string ToString(const llvm::Type &Ty);
std::string ToString(const llvm::Value &V);
std::string ToString(const llvm::MDNode &N);

std::string TypeIDStr(const llvm::Type &Ty);
std::string ValueIDStr(const llvm::Value &V);

void dump(llvm::Module &M, const std::string &File);
void dump(llvm::Function &F, const std::string &File);
void dump(const llvm::MemoryBuffer &MB, const std::string &File);

size_t demotePHINode(llvm::Function &F);
size_t demoteRegs(llvm::Function &F);
size_t reg2mem(llvm::Function &F);

void shuffleFunctions(llvm::Module &M);
bool isModuleGloballyExcluded(llvm::Module *M);
bool isFunctionGloballyExcluded(llvm::Function *F);

[[noreturn]] void fatalError(const char *Msg);
[[noreturn]] void fatalError(const std::string &Msg);

llvm::Expected<std::unique_ptr<llvm::Module>>
generateModule(llvm::StringRef Routine, const llvm::Triple &Triple,
               llvm::StringRef Extension, llvm::LLVMContext &Ctx,
               llvm::ArrayRef<std::string> ExtraArgs);

struct ObfuscationConfig;

class IRChangesMonitor {
public:
  IRChangesMonitor(const llvm::Module &M, llvm::StringRef PassName);

  // Call this function whenever a transformation in the pass reported changes
  // explicitly. This determines the PreservedAnalyses flag returned from the
  // report() function.
  void notify(bool TransformationReportedChange) {
    ChangeReported |= TransformationReportedChange;
  }

  llvm::PreservedAnalyses report();

  IRChangesMonitor(IRChangesMonitor &&) = delete;
  IRChangesMonitor(const IRChangesMonitor &) = delete;
  IRChangesMonitor &operator=(IRChangesMonitor &&) = delete;
  IRChangesMonitor &operator=(const IRChangesMonitor &) = delete;

private:
  const llvm::Module &M;
  ObfuscationConfig *UserConfig;
  std::string PassName;
  std::string OriginalIR;
  bool ChangeReported;
};

class RandomGenerator {
private:
  static bool Seeded;

public:
  static int generate();
  static int checkProbability(int Target);
};

} // end namespace omvll
