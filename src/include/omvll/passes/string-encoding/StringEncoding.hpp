#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include <variant>

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/Triple.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Support/RandomNumberGenerator.h"

#include "omvll/passes/string-encoding/StringEncodingOpt.hpp"

// Forward declarations
namespace llvm {
class ConstantDataSequential;
class GlobalVariable;
class CallInst;
} // end namespace llvm

namespace omvll {

struct ObfuscationConfig;
class Jitter;

// See https://obfuscator.re/omvll/passes/strings-encoding for details.
struct StringEncoding : llvm::PassInfoMixin<StringEncoding> {
  using EncRoutineFn = void (*)(uint8_t *Out, const char *In, uint64_t Key,
                                int Len);
  using KeyBufferTy = std::vector<uint8_t>;
  using KeyIntTy = uint64_t;
  using KeyTy = std::variant<std::monostate, KeyBufferTy, KeyIntTy>;

  enum EncodingTy {
    None = 0,
    Local,
    Global,
    Replace,
  };

  struct EncodingInfo {
    EncodingInfo() = delete;
    EncodingInfo(EncodingTy Ty) : Type(Ty) {};

    EncodingTy Type = EncodingTy::None;
    KeyTy Key;
    std::unique_ptr<llvm::Module> TM;
    std::unique_ptr<llvm::Module> HM;
  };

  llvm::PreservedAnalyses run(llvm::Module &M,
                              llvm::ModuleAnalysisManager &MAM);
  static bool isRequired() { return true; }

  bool encodeStrings(llvm::Function &F, ObfuscationConfig &UserConfig);
  bool injectDecoding(llvm::Instruction &I, llvm::Use &Op,
                      llvm::GlobalVariable &G,
                      llvm::ConstantDataSequential &Data,
                      const EncodingInfo &Info);
  bool injectDecodingLocally(llvm::Instruction &I, llvm::Use &Op,
                             llvm::GlobalVariable &G,
                             llvm::ConstantDataSequential &Data,
                             const EncodingInfo &Info);
  bool process(llvm::Instruction &I, llvm::Use &Op, llvm::GlobalVariable &G,
               llvm::ConstantDataSequential &Data, StringEncodingOpt &Opt);
  bool processReplace(llvm::Instruction &I, llvm::Use &Op,
                      llvm::GlobalVariable &G,
                      llvm::ConstantDataSequential &Data,
                      StringEncOptReplace &Rep);
  bool processGlobal(llvm::Instruction &I, llvm::Use &Op,
                     llvm::GlobalVariable &G,
                     llvm::ConstantDataSequential &Data);
  bool processLocal(llvm::Instruction &I, llvm::Use &Op,
                    llvm::GlobalVariable &G,
                    llvm::ConstantDataSequential &Data);

  inline EncodingInfo *getEncoding(const llvm::GlobalVariable &GV) {
    if (auto It = GVarEncInfo.find(&GV); It != GVarEncInfo.end())
      return &It->second;
    return nullptr;
  }

private:
  static inline Jitter *HostJIT = nullptr;

  void genRoutines(const llvm::Triple &Triple, EncodingInfo &EI,
                   llvm::LLVMContext &Ctx);
  void annotateRoutine(llvm::Module &M);

  std::vector<llvm::CallInst *> ToInline;
  std::vector<llvm::Function *> Ctors;
  std::unique_ptr<llvm::RandomNumberGenerator> RNG;
  llvm::SmallSet<llvm::GlobalVariable *, 10> Obf;
  llvm::DenseMap<llvm::ConstantDataSequential *, std::vector<uint8_t>> KeyMap;
  llvm::DenseMap<llvm::GlobalVariable *, EncodingInfo> GVarEncInfo;
};

} // end namespace omvll
