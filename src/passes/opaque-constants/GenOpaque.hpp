#ifndef OMVLL_GEN_OPAQUE_H
#define OMVLL_GEN_OPAQUE_H
namespace llvm {
class Value;
class Instruction;
class OpaqueContext;
class ConstantInt;
class RandomNumberGenerator;
class Type;
}
namespace omvll {
struct OpaqueContext;

// Opaque ZERO generator
llvm::Value* GetOpaqueZero_1(llvm::Instruction& I, OpaqueContext& C, llvm::Type* Ty, llvm::RandomNumberGenerator& RNG);
llvm::Value* GetOpaqueZero_2(llvm::Instruction& I, OpaqueContext& C, llvm::Type* Ty, llvm::RandomNumberGenerator& RNG);
llvm::Value* GetOpaqueZero_3(llvm::Instruction& I, OpaqueContext& C, llvm::Type* Ty, llvm::RandomNumberGenerator& RNG);

// Opaque ONE generator
llvm::Value* GetOpaqueOne_1(llvm::Instruction& I, OpaqueContext& C, llvm::Type* Ty, llvm::RandomNumberGenerator& RNG);
llvm::Value* GetOpaqueOne_2(llvm::Instruction& I, OpaqueContext& C, llvm::Type* Ty, llvm::RandomNumberGenerator& RNG);
llvm::Value* GetOpaqueOne_3(llvm::Instruction& I, OpaqueContext& C, llvm::Type* Ty, llvm::RandomNumberGenerator& RNG);

// Opaque VALUE != {0, 1} generator
llvm::Value* GetOpaqueConst_1(llvm::Instruction& I, OpaqueContext& C, const llvm::ConstantInt& Val, llvm::RandomNumberGenerator& RNG);
llvm::Value* GetOpaqueConst_2(llvm::Instruction& I, OpaqueContext& C, const llvm::ConstantInt& Val, llvm::RandomNumberGenerator& RNG);
llvm::Value* GetOpaqueConst_3(llvm::Instruction& I, OpaqueContext& C, const llvm::ConstantInt& Val, llvm::RandomNumberGenerator& RNG);
}
#endif
