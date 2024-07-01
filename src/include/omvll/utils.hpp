#ifndef OMVLL_UTILS_H
#define OMVLL_UTILS_H

#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Triple.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/Error.h"
#include <string>

namespace llvm {
class Instruction;
class BasicBlock;
class Function;
class Module;
class Type;
class Value;
class MDNode;
class MemoryBuffer;
}
namespace omvll {
std::string ToString(const llvm::Module& M);
std::string ToString(const llvm::BasicBlock& BB);
std::string ToString(const llvm::Instruction& I);
std::string ToString(const llvm::Type& Ty);
std::string ToString(const llvm::Value& V);

std::string ToString(const llvm::MDNode& N);

std::string TypeIDStr(const llvm::Type& Ty);
std::string ValueIDStr(const llvm::Value& V);

void dump(llvm::Module& M, const std::string& file);
void dump(llvm::Function& F, const std::string& file);
void dump(const llvm::MemoryBuffer& MB, const std::string& file);

size_t demotePHINode(llvm::Function& F);
size_t demoteRegs(llvm::Function& F);
size_t reg2mem(llvm::Function& F);

void shuffleFunctions(llvm::Module& M);

[[noreturn]] void fatalError(const char* msg);
[[noreturn]] void fatalError(const std::string& msg);

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
  const llvm::Module &Mod;
  ObfuscationConfig *UserConfig = nullptr;
  std::string PassName;
  std::string OriginalIR;
  bool ChangeReported;
};
}
#endif
