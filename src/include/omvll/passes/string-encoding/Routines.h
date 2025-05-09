namespace omvll {

using EncRoutineFn = void(char *, const char *, unsigned long long, int);
EncRoutineFn *getEncodeRoutine(unsigned Idx);
const char *getDecodeRoutine(unsigned Idx);
unsigned getNumEncodeDecodeRoutines();

} // namespace omvll
