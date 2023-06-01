LLVM_TARGET="AArch64;X86"

echo "LLVM Android Version: $(git --git-dir=/LLVM/.git describe --dirty)"

cmake -GNinja -S /LLVM/llvm -B /tmp/build_llvm_ndk                                                                   \
      -DCMAKE_CXX_COMPILER=clang++-14 \
      -DCMAKE_C_COMPILER=clang-14 \
      -DCMAKE_CXX_FLAGS="-stdlib=libc++" \
      -DCMAKE_BUILD_TYPE=Release                                                                        \
      -DCMAKE_INSTALL_PREFIX=/llvm-ndk-install                                              \
      -DLLVM_ENABLE_LTO=OFF                                                                            \
      -DLLVM_ENABLE_TERMINFO=OFF                                                                        \
      -DLLVM_ENABLE_THREADS=ON                                                                          \
      -DLLVM_USE_NEWPM=ON                                                                               \
      -DLLVM_VERSION_PATCH=6                           \
      -DLLVM_LIBDIR_SUFFIX=64                                  \
      -DLLVM_TARGET_ARCH=${LLVM_TARGET}                                                                 \
      -DLLVM_TARGETS_TO_BUILD=${LLVM_TARGET}                                                            \
      -DLLVM_ENABLE_PROJECTS="clang;llvm"

ninja -C /tmp/build_llvm_ndk package
cp /tmp/build_llvm_ndk/LLVM-14.0.6git-Linux.tar.gz /LLVM/
