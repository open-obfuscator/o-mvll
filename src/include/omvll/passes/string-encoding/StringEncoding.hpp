#ifndef OMVLL_STRING_ENCODING_H
#define OMVLL_STRING_ENCODING_H
#include "omvll/passes/string-encoding/StringEncodingOpt.hpp"

#include "llvm/Support/RandomNumberGenerator.h"
#include "llvm/IR/PassManager.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallSet.h"
#include <variant>

namespace llvm {
class ConstantDataSequential;
class GlobalVariable;
class CallInst;
}


namespace omvll {
struct ObfuscationConfig;
class Jitter;

//! See: https://obfuscator.re/omvll/passes/strings-encoding for the details
struct StringEncoding : llvm::PassInfoMixin<StringEncoding> {
  using enc_routine_t = void(*)(uint8_t* out, const char* in, uint64_t key, int len);
  enum EncodingTy {
    NONE = 0,
    STACK,
    STACK_LOOP,
    GLOBAL,
    REPLACE,
  };
  using key_buffer_t = std::vector<uint8_t>;
  using key_int_t    = uint64_t;
  using KeyTy = std::variant<std::monostate, key_buffer_t, key_int_t>;
  struct EncodingInfo {
    EncodingInfo() = delete;
    EncodingInfo(EncodingTy Ty) : type(Ty) {};

    EncodingTy type = EncodingTy::NONE;
    KeyTy key;
    std::unique_ptr<llvm::Module> TM;
    std::unique_ptr<llvm::Module> HM;
  };

  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager&);
  static bool isRequired() { return true; }


  bool runOnBasicBlock(llvm::Module& M, llvm::Function& F, llvm::BasicBlock& BB,
                       ObfuscationConfig& userConfig);

  bool injectDecoding(llvm::BasicBlock& BB, llvm::Instruction& I, llvm::Use& OP,
                      llvm::GlobalVariable& G, llvm::ConstantDataSequential& data,
                      const EncodingInfo& info);

  bool injectOnStack(llvm::BasicBlock& BB, llvm::Instruction& I, llvm::Use& OP,
                     llvm::GlobalVariable& G, llvm::ConstantDataSequential& data,
                     const EncodingInfo& info);

  bool injectOnStackLoop(llvm::BasicBlock& BB, llvm::Instruction& I, llvm::Use& OP,
                         llvm::GlobalVariable& G, llvm::ConstantDataSequential& data,
                         const EncodingInfo& info);

  bool process(llvm::BasicBlock& BB, llvm::Instruction& I, llvm::Use& OP,
               llvm::GlobalVariable& G, llvm::ConstantDataSequential& data,
               StringEncodingOpt& opt);

  bool processReplace(llvm::BasicBlock& BB, llvm::Instruction& I, llvm::Use& OP,
                      llvm::GlobalVariable& G,
                      llvm::ConstantDataSequential& data, StringEncOptReplace& rep);

  bool processGlobal(llvm::BasicBlock& BB, llvm::Instruction& I, llvm::Use& OP,
                     llvm::GlobalVariable& G,
                     llvm::ConstantDataSequential& data, StringEncOptGlobal& global);

  bool processOnStack(llvm::BasicBlock& BB, llvm::Instruction& I, llvm::Use& OP,
                      llvm::GlobalVariable& G,
                      llvm::ConstantDataSequential& data, const StringEncOptStack& stack);

  bool processOnStackLoop(llvm::BasicBlock& BB, llvm::Instruction& I, llvm::Use& OP,
                          llvm::GlobalVariable& G, llvm::ConstantDataSequential& data);

  inline EncodingInfo* getEncoding(const llvm::GlobalVariable& GV) {
    if (auto it = gve_info_.find(&GV); it != gve_info_.end()) {
      return &it->second;
    }
    return nullptr;
  }

private:
  inline static Jitter* HOSTJIT = nullptr;

  void genRoutines(const std::string& Triple, EncodingInfo& EI, llvm::LLVMContext& Ctx);
  void annotateRoutine(llvm::Module& M);

  std::vector<llvm::CallInst*> inline_wlist_;
  std::vector<llvm::Function*> ctor_;
  std::unique_ptr<llvm::RandomNumberGenerator> RNG_;
  llvm::SmallSet<llvm::GlobalVariable*, 10> obf_;
  llvm::DenseMap<llvm::ConstantDataSequential*, std::vector<uint8_t>> key_map_;

  llvm::DenseMap<llvm::GlobalVariable*, EncodingInfo> gve_info_;
};


}

#endif
