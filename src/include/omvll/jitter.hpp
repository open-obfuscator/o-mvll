#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include <memory>
#include <string>

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Error.h"

// Forward declarations
namespace llvm {
class LLVMContext;
class Module;
class MemoryBuffer;

namespace orc {
class LLJIT;
} // end namespace orc

namespace object {
class ObjectFile;
} // end namespace object
} // end namespace llvm

namespace omvll {

class Jitter {
public:
  Jitter(const std::string &Triple);

  llvm::LLVMContext &getContext() { return *Ctx; }

  llvm::Expected<std::unique_ptr<llvm::Module>>
  loadModule(llvm::StringRef Path, llvm::LLVMContext &Ctx);

  std::unique_ptr<llvm::orc::LLJIT> compile(llvm::Module &M);
  std::unique_ptr<llvm::MemoryBuffer> jitAsm(const std::string &Asm,
                                             size_t Size);

protected:
  size_t getFunctionSize(llvm::object::ObjectFile &Obj, llvm::StringRef Name);

private:
  std::string Triple;
  std::unique_ptr<llvm::LLVMContext> Ctx;

  bool hasInitializedARMAssembler = false;
  void initializeARMAssembler();
  bool hasInitializedAArch64Assembler = false;
  void initializeAArch64Assembler();
};

} // end namespace omvll
