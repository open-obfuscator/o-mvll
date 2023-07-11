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
  llvm::LLVMContext& getContext() { return *Ctx_; }

  llvm::Expected<std::unique_ptr<llvm::Module>>
  loadModule(llvm::StringRef Path);
  llvm::Expected<std::unique_ptr<llvm::Module>>
  loadModule(llvm::StringRef Path, llvm::LLVMContext& Ctx);
  std::unique_ptr<llvm::orc::LLJIT> compile(llvm::Module& M);

  std::unique_ptr<llvm::MemoryBuffer> jitAsm(const std::string& Asm, size_t Size);

  static std::unique_ptr<Jitter> Create(const std::string& Triple);
  static std::unique_ptr<Jitter> Create();

protected:
  size_t getFunctionSize(llvm::object::ObjectFile& Obj, llvm::StringRef Name);

private:
  Jitter(const std::string& Triple);

  std::string Triple_;
  std::unique_ptr<llvm::LLVMContext> Ctx_;
};
}
#endif
