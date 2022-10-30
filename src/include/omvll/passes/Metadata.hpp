#ifndef OMVLL_METADATA_H
#define OMVLL_METADATA_H
#include <variant>
#include <llvm/ADT/Optional.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/ADT/ArrayRef.h>

namespace llvm {
class Instruction;
}

namespace omvll {
enum MObfTy {
  NONE = 0,

  OPAQUE_CST,  // Force to apply opaque constants
  OPAQUE_OP,   // Force to apply MBA-like
  OPAQUE_COND, // Force to apply opaque conditions

  PROTECT_FIELD_ACCESS, // Force to apply opaque field access
};

using MObfVal = std::variant<
  std::monostate,
  uint64_t
>;

struct MetaObf {
  inline static constexpr MObfTy None = NONE;

  MetaObf() = default;
  MetaObf(MObfTy Ty) : type(Ty) {}
  MetaObf(MObfTy Ty, MObfVal val) : type(Ty), value(std::move(val)) {}
  MObfTy type = MObfTy::NONE;
  MObfVal value;
  inline bool isNone() { return type == MObfTy::NONE; }
  inline bool hasValue() { return !std::holds_alternative<std::monostate>(value); }
  template<class T>
  const T* get() {
    if (const T* v = std::get_if<T>(&value)) {
      return v;
    }
    return nullptr;
  }
};

void addMetadata(llvm::Instruction& I, MetaObf M);
void addMetadata(llvm::Instruction& I, llvm::ArrayRef<MetaObf> M);
llvm::SmallVector<MetaObf, 5> getObfMetadata(llvm::Instruction& I);
llvm::Optional<MetaObf> getObf(llvm::Instruction& I, MObfTy M);
bool hasObf(llvm::Instruction& I, MObfTy M);

}
#endif
