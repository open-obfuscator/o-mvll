#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

namespace omvll {

using EncRoutineFn = void(char *, const char *, unsigned long long, int);
EncRoutineFn *getEncodeRoutine(unsigned Idx);
const char *getDecodeRoutine(unsigned Idx);
unsigned getNumEncodeDecodeRoutines();

} // end namespace omvll
