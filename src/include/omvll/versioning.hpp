#ifndef OMVLL_VERIONING_H
#define OMVLL_VERIONING_H
#include <llvm/Support/VCSRevision.h>
#include <llvm/Config/llvm-config.h>
#include "omvll/config.hpp"

#define OMVLL_LLVM_VERSION        LLVM_REVISION
#define OMVLL_LLVM_REPO           LLVM_REPOSITORY
#define OMVLL_LLVM_MAJOR          LLVM_VERSION_MAJOR
#define OMVLL_LLVM_MINOR          LLVM_VERSION_MINOR
#define OMVLL_LLVM_PATCH          LLVM_VERSION_PATCH
#define OMVLL_LLVM_VERSION_STRING LLVM_VERSION_STRING

#endif
