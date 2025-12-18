# mirror_hash Development Container
# Builds clang-p2996 with C++26 reflection support
#
# Build time: ~60 minutes (compiling LLVM/Clang from source)
# Disk space: ~20GB during build, ~5GB final image
#
# Usage:
#   docker build -t mirror_hash:latest .
#   docker run -it -v $(pwd):/workspace mirror_hash:latest

FROM ubuntu:22.04

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install essential build tools
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    wget \
    ca-certificates \
    python3 \
    python3-pip \
    vim \
    && rm -rf /var/lib/apt/lists/*

# Build and install clang-p2996 with reflection support
WORKDIR /opt

# Clone Bloomberg's clang-p2996 branch (C++26 reflection implementation)
RUN git clone --depth 1 --branch p2996 https://github.com/bloomberg/clang-p2996.git

# Build clang with reflection support
WORKDIR /opt/clang-p2996/build
RUN cmake -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_ENABLE_PROJECTS="clang" \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    ../llvm && \
    ninja && \
    ninja install

# Set up compiler environment variables
ENV CC=/usr/local/bin/clang
ENV CXX=/usr/local/bin/clang++

# Build libc++ with reflection support (required for <meta> header)
WORKDIR /opt/clang-p2996/runtimes-build
RUN cmake -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi;libunwind" \
    -DLIBCXX_ENABLE_REFLECTION=ON \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DCMAKE_C_COMPILER=/usr/local/bin/clang \
    -DCMAKE_CXX_COMPILER=/usr/local/bin/clang++ \
    ../runtimes && \
    ninja && \
    ninja install

# Clean up build artifacts to reduce image size
RUN rm -rf /opt/clang-p2996

# Configure library paths
ENV LD_LIBRARY_PATH=/usr/local/lib:/usr/local/lib/aarch64-unknown-linux-gnu:/usr/local/lib/x86_64-unknown-linux-gnu:$LD_LIBRARY_PATH

# Verify <meta> header is installed and reflection works
RUN echo '#include <meta>' > /tmp/test.cpp && \
    echo 'int main() { return 0; }' >> /tmp/test.cpp && \
    clang++ -std=c++2c -freflection -freflection-latest -stdlib=libc++ /tmp/test.cpp -o /tmp/test && \
    rm /tmp/test.cpp /tmp/test && \
    echo "SUCCESS: C++26 reflection is available"

WORKDIR /workspace
CMD ["/bin/bash"]
