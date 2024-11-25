#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "llvm/Config/llvm-config.h"
#include "llvm/Support/VCSRevision.h"

#include "omvll/config.hpp"

#ifdef LLVM_REVISION
#define OMVLL_LLVM_VERSION        " (" LLVM_REVISION ")"
#else
#define OMVLL_LLVM_VERSION
#endif

#define OMVLL_LLVM_REPO           LLVM_REPOSITORY
#define OMVLL_LLVM_MAJOR          LLVM_VERSION_MAJOR
#define OMVLL_LLVM_MINOR          LLVM_VERSION_MINOR
#define OMVLL_LLVM_PATCH          LLVM_VERSION_PATCH
#define OMVLL_LLVM_VERSION_STRING LLVM_VERSION_STRING
