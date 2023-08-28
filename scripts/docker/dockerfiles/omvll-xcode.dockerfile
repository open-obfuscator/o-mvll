# Docker file used to build O-MVLL pass plugin for the Android NDK
# Stage 1: Docker for building the toolchain
# docker build \
#    --target osxcross-builder \
#    -t openobfuscator/omvll-osx-builder -f ./omvll-xcode.dockerfile .
# Stage 2: Docker for using the toolchain
# docker build \
#    --target xcode \
#    -t openobfuscator/omvll-xcode -f ./omvll-xcode.dockerfile .

FROM debian:bookworm-20220912-slim AS osxcross-builder
LABEL maintainer="Romain Thomas <me@romainthomas.fr>"

COPY MacOSX13.0.sdk.tar.xz /

RUN mkdir -p /usr/share/man/man1 && \
    apt-get update -y && \
    apt-get install -y --no-install-recommends \
    ninja-build g++ gcc clang cmake git make ca-certificates curl unzip python3 \
    libssl-dev libxml2-dev patch zlib1g-dev xz-utils \
  && apt-get clean \
  && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /osxcross && cd /osxcross && \
    git clone --depth 1 https://github.com/tpoechtrager/osxcross && \
    cd osxcross && \
    ln -s /MacOSX13.0.sdk.tar.xz tarballs/ && \
    TARGET_DIR=/osxcross/install UNATTENDED=1 bash ./build.sh

FROM debian:bookworm-20220912-slim AS xcode
COPY --from=osxcross-builder /osxcross/install  /osxcross
RUN mkdir -p /usr/share/man/man1 && \
    apt-get update -y && \
    apt-get install -y --no-install-recommends \
    ninja-build clang cmake git make ca-certificates curl unzip zlib1g-dev xz-utils python3 \
  && apt-get clean \
  && rm -rf /var/lib/apt/lists/*

ENV PATH="/osxcross/bin:${PATH}"
ENV LD_LIBRARY_PATH="/osxcross/lib:${LD_LIBRARY_PATH}"

