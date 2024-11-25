//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "llvm/IR/Constants.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"

#include "omvll/passes/Metadata.hpp"
#include "omvll/visitvariant.hpp"

using namespace llvm;

static constexpr auto ObfKey = "obf";

namespace omvll {

void addMetadata(Instruction &I, MetaObf M) {
  addMetadata(I, ArrayRef<MetaObf>(M));
}

Metadata *serialize(LLVMContext &C, const MetaObf &MObf) {
  ConstantAsMetadata *Ty = ConstantAsMetadata::get(
      ConstantInt::get(Type::getInt32Ty(C), MObf.Type, /* signed ? */ false));

  Metadata *Value =
      std::visit(overloaded{
                     [](std::monostate) -> Metadata * { return nullptr; },
                     [&C](uint64_t value) -> Metadata * {
                       return ConstantAsMetadata::get(ConstantInt::get(
                           Type::getInt64Ty(C), value, /* signed ? */ false));
                     },
                 },
                 MObf.Value);

  if (Value)
    return MDTuple::get(C, {Ty, Value});
  return MDTuple::get(C, {Ty});
}

MetaObf deserialize(LLVMContext &C, const Metadata &Meta) {
  auto *Root = dyn_cast<MDTuple>(&Meta);
  if (!Root)
    return MetaObfTy::None;

  if (Root->getNumOperands() < 1)
    return MetaObfTy::None;

  const MDOperand &OpType = Root->getOperand(0);
  auto *CM = dyn_cast<ConstantAsMetadata>(OpType.get());
  if (!CM)
    return MetaObfTy::None;

  auto *CI = dyn_cast<ConstantInt>(CM->getValue());
  if (!CI)
    return MetaObfTy::None;

  auto T = MetaObfTy(CI->getLimitedValue());
  MetaObf MO(T);

  if (Root->getNumOperands() > 1) {
    const MDOperand &Val = Root->getOperand(1);
    if (auto *CM = dyn_cast<ConstantAsMetadata>(Val.get()))
      if (auto *CI = dyn_cast<ConstantInt>(CM->getValue()))
        MO.Value = CI->getLimitedValue();
  }
  return MO;
}

void addMetadata(Instruction &I, ArrayRef<MetaObf> M) {
  LLVMContext &Ctx = I.getContext();
  SmallVector<Metadata *, 5> Metadata;
  for (auto MObf : M)
    Metadata.push_back(serialize(Ctx, MObf));

  MDTuple *Node = MDTuple::get(Ctx, Metadata);
  I.setMetadata(ObfKey, Node);
}

SmallVector<MetaObf, 5> getObfMetadata(Instruction &I) {
  LLVMContext &Ctx = I.getContext();
  SmallVector<MetaObf, 5> Result;
  if (auto *Node = I.getMetadata(ObfKey))
    for (const MDOperand &Op : Node->operands())
      if (auto MO = deserialize(Ctx, *Op); !MO.isNone())
        Result.push_back(std::move(MO));
  return Result;
}

std::optional<MetaObf> getObf(Instruction &I, MetaObfTy M) {
  auto *Node = I.getMetadata(ObfKey);
  if (!Node)
    return std::nullopt;

  for (const MDOperand &Op : Node->operands()) {
    MetaObf MO = deserialize(I.getContext(), *Op);
    if (MO.Type == M)
      return MO;
  }
  return std::nullopt;
}

bool hasObf(Instruction &I, MetaObfTy M) {
  std::optional<MetaObf> Obf = getObf(I, M);
  if (Obf.has_value())
    return Obf->hasValue() && !(Obf->isNone());
  return false;
}

} // end namespace omvll
