FROM debian:stretch-slim
LABEL maintainer="Romain Thomas <me@romainthomas.fr>"
RUN mkdir -p /usr/share/man/man1                                      && \
    apt-get update -y                                                 && \
    apt-get install -y --no-install-recommends                           \
    ninja-build g++ gcc clang-11 git make ca-certificates curl unzip     \
    build-essential zlib1g-dev libncurses5-dev libgdbm-dev libnss3-dev   \
    libreadline-dev libffi-dev libsqlite3-dev wget libbz2-dev            \
    libc++abi-11-dev libc++-11-dev                                       \
  && apt-get clean                                                       \
  && rm -rf /var/lib/apt/lists/*

# Python 3.10.7 requires at least OpenSSL 1.1.1 while debian:stretch-slim provides OpenSSL 1.1.0.
# Hence, we need to compile this version
RUN curl -LO https://www.openssl.org/source/openssl-1.1.1q.tar.gz && \
    tar xzvf ./openssl-1.1.1q.tar.gz                              && \
    cd openssl-1.1.1q                                             && \
    CC=clang-11 CXX=clang++-11 CFLAGS="-fPIC"                        \
      ./config --prefix=/usr --openssldir=/usr shared             && \
    make -j$(nproc)                                               && \
    make -j$(nproc) install                                       && \
    mv /usr/lib/libcrypto.so.1.1 /usr/lib/x86_64-linux-gnu/       && \
    mv /usr/lib/libssl.so.1.1    /usr/lib/x86_64-linux-gnu/       && \
    cd .. && rm -rf openssl-1.1.1q && rm -rf openssl-1.1.1q.tar.gz

# Install a recent version of cmake
RUN curl -LO https://github.com/Kitware/CMake/releases/download/v3.24.2/cmake-3.24.2-linux-x86_64.sh && \
    chmod u+x ./cmake-3.24.2-linux-x86_64.sh                                                         && \
    ./cmake-3.24.2-linux-x86_64.sh --skip-license --prefix=/usr/                                     && \
    rm -rf ./cmake-3.24.2-linux-x86_64.sh

# Install a recent version of python3
RUN curl -LO https://www.python.org/ftp/python/3.10.7/Python-3.10.7.tgz && \
    tar xzvf Python-3.10.7.tgz                                          && \
    cd Python-3.10.7                                                    && \
    CC=clang-11 CXX=clang++-11 CFLAGS="-fPIC -m64"                         \
    ./configure                                                            \
      --enable-shared                                                      \
      --disable-ipv6                                                       \
      --host=x86_64-linux-gnu                                              \
      --target=x86_64-linux-gnu                                            \
      --build=x86_64-linux-gnu                                             \
      --disable-test-modules                                            && \
    make -j$(nproc) install                                             && \
    mv /usr/local/lib/libpython3.10.so.1.0 /usr/lib/x86_64-linux-gnu/   && \
    cd .. && rm -rf Python-3.10.7.tgz && rm -rf Python-3.10.7
    