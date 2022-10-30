#ifndef OMVLL_JITTER_H
#define OMVLL_JITTER_H
#include <string>
#include <memory>

#include <llvm/ADT/IntrusiveRefCntPtr.h>
#include <llvm/ADT/StringRef.h>

namespace llvm {
class LLVMContext;
class Module;
class MemoryBuffer;
namespace vfs {
class InMemoryFileSystem;
}
namespace orc {
class LLJIT;
}

namespace object {
class ObjectFile;
}
}

namespace clang {
class CompilerInstance;
class DiagnosticIDs;
class DiagnosticOptions;
class DiagnosticsEngine;
class TextDiagnosticPrinter;
class FileManager;
namespace driver {
class Driver;
}
}


namespace omvll {
class Jitter {
  public:
  std::unique_ptr<llvm::Module> generate(const std::string& code);
  std::unique_ptr<llvm::Module> generate(const std::string& code, llvm::LLVMContext& Ctx);
  std::unique_ptr<llvm::orc::LLJIT> compile(llvm::Module& M);

  std::unique_ptr<llvm::MemoryBuffer> jitAsm(const std::string& Asm, size_t Size);

  static std::unique_ptr<Jitter> Create(const std::string& Triple);
  static std::unique_ptr<Jitter> Create();

  protected:
  size_t getFunctionSize(llvm::object::ObjectFile& Obj, llvm::StringRef Name);

  private:
  Jitter(const std::string& Triple);
  Jitter();
  std::string Triple_;
  std::unique_ptr<clang::driver::Driver> Driver_;
  std::unique_ptr<clang::CompilerInstance> Clang_;
  llvm::IntrusiveRefCntPtr<clang::DiagnosticsEngine> Diags_;
  llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs> DiagID_;
  llvm::IntrusiveRefCntPtr<clang::DiagnosticOptions> DiagOpts_;

  llvm::IntrusiveRefCntPtr<llvm::vfs::InMemoryFileSystem> VFS_;
  llvm::IntrusiveRefCntPtr<clang::FileManager> FileMgr_;
  std::unique_ptr<llvm::LLVMContext> Ctx_;
};
}
#endif
