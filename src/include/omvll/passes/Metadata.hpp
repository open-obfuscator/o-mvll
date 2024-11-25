#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include <optional>
#include <variant>

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"

// Forward declarations
namespace llvm {
class Instruction;
} // end namespace llvm

namespace omvll {

enum MetaObfTy {
  None = 0,
  OpaqueCst,          // Force to apply opaque constants.
  OpaqueOp,           // Force to apply MBA-like.
  OpaqueCond,         // Force to apply opaque conditions.
  ProtectFieldAccess, // Force to apply opaque field access.
};

using MetaObfVal = std::variant<std::monostate, uint64_t>;

struct MetaObf {
  MetaObf() = default;
  MetaObf(MetaObfTy Ty) : Type(Ty) {}
  MetaObf(MetaObfTy Ty, MetaObfVal Value) : Type(Ty), Value(std::move(Value)) {}
  MetaObfTy Type = MetaObfTy::None;
  MetaObfVal Value;

  inline bool isNone() { return Type == MetaObfTy::None; }
  inline bool hasValue() {
    return !std::holds_alternative<std::monostate>(Value);
  }

  template <class T> const T *get() {
    if (const T *V = std::get_if<T>(&Value))
      return V;
    return nullptr;
  }
};

void addMetadata(llvm::Instruction &I, MetaObf M);
void addMetadata(llvm::Instruction &I, llvm::ArrayRef<MetaObf> M);
llvm::SmallVector<MetaObf, 5> getObfMetadata(llvm::Instruction &I);
std::optional<MetaObf> getObf(llvm::Instruction &I, MetaObfTy M);
bool hasObf(llvm::Instruction &I, MetaObfTy M);

} // end namespace omvll
