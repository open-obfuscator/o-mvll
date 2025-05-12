//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "omvll/passes/string-encoding/Routines.h"
#include <cassert>

namespace {

const char *DecodeRoutines[] = {
  R"delim(
    void decode(char *out, char *in, unsigned long long key, int size) {
      unsigned char *raw_key = (unsigned char*)(&key);
      for (int i = 0; i < size; ++i) {
        out[i] = in[i] ^ raw_key[i % sizeof(key)];
      }
    }
  )delim",
  R"delim(
    void decode(char *out, char *in, unsigned long long key, int size) {
      unsigned char *raw_key = (unsigned char*)(&key);
      for (int i = 0; i < size; ++i) {
        out[i] = in[i] ^ raw_key[i % sizeof(key)] ^ i;
      }
    }
  )delim",
};

omvll::EncRoutineFn *EncodeRoutines[] = {
    [](char *out, const char *in, unsigned long long key, int size) {
      unsigned char *raw_key = (unsigned char *)(&key);
      for (int i = 0; i < size; ++i) {
        out[i] = in[i] ^ raw_key[i % sizeof(key)];
      }
    },
    [](char *out, const char *in, unsigned long long key, int size) {
      unsigned char *raw_key = (unsigned char *)(&key);
      for (int i = 0; i < size; ++i) {
        out[i] = in[i] ^ raw_key[i % sizeof(key)] ^ i;
      }
    }};

} // end anonymous namespace

namespace omvll {

template <typename T, unsigned N>
static constexpr unsigned arraySize(const T (&)[N]) noexcept {
  return N;
}

// Encode and decode functions must match in pairs
static_assert(arraySize(EncodeRoutines) == arraySize(DecodeRoutines));

unsigned getNumEncodeDecodeRoutines() { return arraySize(EncodeRoutines); }

EncRoutineFn *getEncodeRoutine(unsigned Idx) {
  assert(Idx < arraySize(EncodeRoutines));
  return EncodeRoutines[Idx];
}

const char *getDecodeRoutine(unsigned Idx) {
  assert(Idx < arraySize(DecodeRoutines));
  return DecodeRoutines[Idx];
}

} // end namespace omvll
