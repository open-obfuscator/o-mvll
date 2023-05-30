FROM debian:stretch-slim
LABEL maintainer="Romain Thomas <me@romainthomas.fr>"
RUN mkdir -p /usr/share/man/man1                                      && \
    apt-get update -y                                                 && \
    apt-get install -y --no-install-recommends                           \
    ninja-build g++ gcc clang-14 git make ca-certificates curl unzip     \
    build-essential zlib1g-dev libncurses5-dev libgdbm-dev libnss3-dev   \
    libreadline-dev libffi-dev libsqlite3-dev wget libbz2-dev            \
    libc++abi-14-dev libc++-14-dev llvm-14-tools                         \
    python3 python3-pip                                                  \
  && apt-get clean                                                       \
  && rm -rf /var/lib/apt/lists/*

# Install a recent version of cmake
RUN curl -LO https://github.com/Kitware/CMake/releases/download/v3.24.2/cmake-3.24.2-linux-x86_64.sh && \
    chmod u+x ./cmake-3.24.2-linux-x86_64.sh                                                         && \
    ./cmake-3.24.2-linux-x86_64.sh --skip-license --prefix=/usr/                                     && \
    rm -rf ./cmake-3.24.2-linux-x86_64.sh

RUN python3 -m pip install lit && lit --version
RUN git config --global --add safe.directory /o-mvll

# Copy in missing test dependencies from a local resource
# FIXME: Once we update the deps-package, these 3 must be included
RUN mkdir -p /test-deps/bin
COPY llvm-config /test-deps/bin
COPY clang /test-deps/bin
RUN ln -s /test-deps/bin/clang /test-deps/bin/clang++

# FIXME: Once we have the above binaries in the deps-package, we should be able
#        to point LLVM_TOOLS_DIR=/usr/lib/llvm-14 and remove this as well
RUN cd /usr/lib/llvm-14/bin && cp FileCheck count not /test-deps/bin/. && cd /
