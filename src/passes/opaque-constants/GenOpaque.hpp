#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

// Forward declarations
namespace llvm {
class Value;
class Instruction;
class OpaqueContext;
class ConstantInt;
class RandomNumberGenerator;
class Type;
} // end namespace llvm

namespace omvll {

struct OpaqueContext;

// Opaque Zero generator.
llvm::Value *getOpaqueZero1(llvm::Instruction &I, llvm::Type *Ty, llvm::RandomNumberGenerator &RNG);
llvm::Value *getOpaqueZero2(llvm::Instruction &I, llvm::Type *Ty, llvm::RandomNumberGenerator &RNG);
llvm::Value *getOpaqueZero3(llvm::Instruction &I, llvm::Type *Ty, llvm::RandomNumberGenerator &RNG);

// Opaque One generator.
llvm::Value *getOpaqueOne1(llvm::Instruction &I, llvm::Type *Ty, llvm::RandomNumberGenerator &RNG);
llvm::Value *getOpaqueOne2(llvm::Instruction &I, llvm::Type *Ty, llvm::RandomNumberGenerator &RNG);
llvm::Value *getOpaqueOne3(llvm::Instruction &I, llvm::Type *Ty, llvm::RandomNumberGenerator &RNG);

// Opaque Value != {0, 1} generator.
llvm::Value *getOpaqueConst1(llvm::Instruction &I, const llvm::ConstantInt &Val,
                             llvm::RandomNumberGenerator &RNG);
llvm::Value *getOpaqueConst2(llvm::Instruction &I, const llvm::ConstantInt &Val,
                             llvm::RandomNumberGenerator &RNG);
llvm::Value *getOpaqueConst3(llvm::Instruction &I, const llvm::ConstantInt &Val,
                             llvm::RandomNumberGenerator &RNG);

} // end namespace omvll
