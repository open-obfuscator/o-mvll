FROM debian:stable-slim
LABEL maintainer="Romain Thomas <me@romainthomas.fr>"

COPY MacOSX13.0.sdk.tar.xz /

RUN mkdir -p /usr/share/man/man1                                      && \
    apt-get update -y                                                 && \
    apt-get install -y --no-install-recommends                           \
    ninja-build g++ gcc clang clang-14 git make ca-certificates curl unzip     \
    build-essential zlib1g-dev libncurses5-dev libgdbm-dev libnss3-dev   \
    libreadline-dev libffi-dev libsqlite3-dev wget libbz2-dev            \
    libc++abi-14-dev libc++-14-dev llvm-14-tools llvm-14-dev             \
    python3 python3-pip python3-venv libc6-dev cmake libssl-dev libxml2-dev patch xz-utils             \
  && apt-get clean                                                       \
  && rm -rf /var/lib/apt/lists/*

RUN python3 -m venv /opt/venv
ENV PATH="/opt/venv/bin:$PATH"

RUN python3 -m pip install lit && lit --version
RUN git config --global --add safe.directory /o-mvll

# Copy in missing test dependencies
RUN mkdir -p /test-deps/bin
RUN ln -s /opt/venv/bin/lit /test-deps/bin/llvm-lit
RUN cd /usr/lib/llvm-14/bin && cp llvm-config FileCheck count not /test-deps/bin/. && cd /

# OSXCross compilation
RUN mkdir -p /osxcross && cd /osxcross && \
    git clone --depth 1 https://github.com/tpoechtrager/osxcross && \
    cd osxcross && \
    ln -s /MacOSX13.0.sdk.tar.xz tarballs/ && \
    TARGET_DIR=/osxcross/install UNATTENDED=1 bash ./build.sh && \
    mv /osxcross/install/* /osxcross


ENV PATH="/osxcross/bin:${PATH}"
ENV LD_LIBRARY_PATH="/osxcross/lib:${LD_LIBRARY_PATH}"
