#ifndef OMVLL_JITTER_H
#define OMVLL_JITTER_H
#include <string>
#include <memory>

#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/Error.h>

namespace llvm {
class LLVMContext;
class Module;
class MemoryBuffer;
namespace orc {
class LLJIT;
}

namespace object {
class ObjectFile;
}
}

namespace omvll {
class Jitter {
public:
  Jitter(const std::string &Triple);

  llvm::LLVMContext &getContext() { return *Ctx_; }

  llvm::Expected<std::unique_ptr<llvm::Module>>
  loadModule(llvm::StringRef Path, llvm::LLVMContext &Ctx);

  std::unique_ptr<llvm::orc::LLJIT> compile(llvm::Module& M);

  std::unique_ptr<llvm::MemoryBuffer> jitAsm(const std::string& Asm, size_t Size);

protected:
  size_t getFunctionSize(llvm::object::ObjectFile& Obj, llvm::StringRef Name);

private:
  std::string Triple_;
  std::unique_ptr<llvm::LLVMContext> Ctx_;
};
}
#endif
