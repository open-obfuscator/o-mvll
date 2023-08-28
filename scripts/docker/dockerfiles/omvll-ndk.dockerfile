FROM debian:stable-slim
LABEL maintainer="Romain Thomas <me@romainthomas.fr>"

RUN mkdir -p /usr/share/man/man1                                      && \
    apt-get update -y                                                 && \
    apt-get install -y --no-install-recommends                           \
    ninja-build g++ gcc clang-14 git make ca-certificates curl unzip     \
    build-essential zlib1g-dev libncurses5-dev libgdbm-dev libnss3-dev   \
    libreadline-dev libffi-dev libsqlite3-dev wget libbz2-dev            \
    libc++abi-14-dev libc++-14-dev llvm-14-tools llvm-14-dev             \
    python3 python3-pip python3-venv libc6-dev                           \
  && apt-get clean                                                       \
  && rm -rf /var/lib/apt/lists/*

# Install a recent version of cmake
RUN curl -LO https://github.com/Kitware/CMake/releases/download/v3.24.2/cmake-3.24.2-linux-x86_64.sh && \
    chmod u+x ./cmake-3.24.2-linux-x86_64.sh                                                         && \
    ./cmake-3.24.2-linux-x86_64.sh --skip-license --prefix=/usr/                                     && \
    rm -rf ./cmake-3.24.2-linux-x86_64.sh

RUN python3 -m venv /opt/venv
ENV PATH="/opt/venv/bin:$PATH"

RUN python3 -m pip install lit && lit --version
RUN git config --global --add safe.directory /o-mvll

# Copy in missing test dependencies
RUN mkdir -p /test-deps/bin
RUN ln -s /opt/venv/bin/lit /test-deps/bin/llvm-lit
RUN cd /usr/lib/llvm-14/bin && cp llvm-config FileCheck count not /test-deps/bin/. && cd /
