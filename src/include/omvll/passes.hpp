#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "omvll/passes/ObfuscationOpt.hpp"
#include "omvll/passes/anti-hook/AntiHook.hpp"
#include "omvll/passes/arithmetic/Arithmetic.hpp"
#include "omvll/passes/basic-block-duplicate/BasicBlockDuplicate.hpp"
#include "omvll/passes/break-cfg/BreakControlFlow.hpp"
#include "omvll/passes/cfg-flattening/ControlFlowFlattening.hpp"
#include "omvll/passes/cleaning/Cleaning.hpp"
#include "omvll/passes/function-outline/FunctionOutline.hpp"
#include "omvll/passes/indirect-branch/IndirectBranch.hpp"
#include "omvll/passes/indirect-call/IndirectCall.hpp"
#include "omvll/passes/logger-bind/LoggerBind.hpp"
#include "omvll/passes/objc-cleaner/ObjCleaner.hpp"
#include "omvll/passes/opaque-constants/OpaqueConstants.hpp"
#include "omvll/passes/opaque-field-access/OpaqueFieldAccess.hpp"
#include "omvll/passes/string-encoding/StringEncoding.hpp"
