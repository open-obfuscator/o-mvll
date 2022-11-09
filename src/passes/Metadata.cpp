#include "omvll/passes/Metadata.hpp"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"

#include "omvll/visitvariant.hpp"

using namespace llvm;

namespace omvll {

static constexpr const char OBF_KEY[] = "obf";

void addMetadata(llvm::Instruction& I, MetaObf M) {
  return addMetadata(I, llvm::ArrayRef<MetaObf>(M));
}

Metadata* serialize(LLVMContext& C, const MetaObf& MObf) {
  ConstantAsMetadata* Ty = ConstantAsMetadata::get(ConstantInt::get(Type::getInt32Ty(C), MObf.type,
                                                                    /* signed ? */false));
  Metadata* value = std::visit(overloaded {
      []   (std::monostate) -> Metadata* { return nullptr; },
      [&C] (uint64_t value) -> Metadata* {
          return ConstantAsMetadata::get(
                  ConstantInt::get(Type::getInt64Ty(C), value, /* signed ? */false));
      },
  }, MObf.value);

  if (value) {
    return MDTuple::get(C, {Ty, value});
  }

  return MDTuple::get(C, {Ty});
}

MetaObf deserialize(LLVMContext& C, const Metadata& Meta) {
  auto* Root = dyn_cast<MDTuple>(&Meta);
  if (Root == nullptr) {
    return MetaObf::None;
  }

  if (Root->getNumOperands() < 1) {
    return MetaObf::None;
  }

  const MDOperand& opType = Root->getOperand(0);
  auto* CM = dyn_cast<ConstantAsMetadata>(opType.get());

  if (CM == nullptr) {
    return MetaObf::None;
  }

  auto* CI = dyn_cast<ConstantInt>(CM->getValue());

  if (CI == nullptr) {
    return MetaObf::None;
  }

  auto T = MObfTy(CI->getLimitedValue());
  MetaObf MO(T);

  if (Root->getNumOperands() > 1) {
    const MDOperand& Val = Root->getOperand(1);
    if (auto* CM = dyn_cast<ConstantAsMetadata>(Val.get())) {
      if (auto* CI = dyn_cast<ConstantInt>(CM->getValue())) {
        MO.value = CI->getLimitedValue();
      }
    }
  }
  return MO;
}

void addMetadata(llvm::Instruction& I, llvm::ArrayRef<MetaObf> M) {
  auto& C = I.getContext();
  llvm::SmallVector<Metadata*, 5> metadata;
  for (auto MObf : M) {
    metadata.push_back(serialize(C, MObf));
  }
  MDTuple* node = MDTuple::get(C, metadata);
  I.setMetadata(OBF_KEY, node);
}



llvm::SmallVector<MetaObf, 5> getObfMetadata(llvm::Instruction& I) {
  LLVMContext& C = I.getContext();
  llvm::SmallVector<MetaObf, 5> result;
  if (auto* node = I.getMetadata(OBF_KEY)) {
    for (const MDOperand& op : node->operands()) {
      if (auto MO = deserialize(C, *op); !MO.isNone()) {
        result.push_back(std::move(MO));
      }
    }
  }
  return result;
}

Optional<MetaObf> getObf(llvm::Instruction& I, MObfTy M) {
  auto* node = I.getMetadata(OBF_KEY);
  if (node == nullptr) {
    return NoneType();
  }
  for (const MDOperand& op : node->operands()) {
    MetaObf MO = deserialize(I.getContext(), *op);
    if (MO.type == M) {
      return MO;
    }
  }
  return NoneType();
}


bool hasObf(llvm::Instruction& I, MObfTy M) {
  Optional<MetaObf> Obf = getObf(I, M);
  return Obf.hasValue() && !(Obf->isNone());
}

}
