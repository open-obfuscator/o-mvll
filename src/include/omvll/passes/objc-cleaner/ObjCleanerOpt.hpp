#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

namespace omvll {

struct ObjCleanerOpt {
  ObjCleanerOpt(bool Value) : Value(Value) {}
  operator bool() const { return Value; }
  bool Value = false;
};

} // end namespace omvll
