FROM nvidia/cuda:12.2.2-devel-ubuntu22.04

# Set environment variables to avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive
ENV CUDA_VISIBLE_DEVICES=all

# Set working directory
WORKDIR /workspace

# Install system dependencies and development tools
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    pkg-config \
    libglfw3-dev \
    libassimp-dev \
    libyaml-cpp-dev \
    libgtest-dev \
    libglm-dev \
    libx11-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxi-dev \
    libgl1-mesa-dev \
    && rm -rf /var/lib/apt/lists/*

# Clone and build Google Test (if needed from source)
RUN cd /usr/src/gtest && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make && \
    cp lib/*.a /usr/lib && \
    cd /workspace && \
    rm -rf /usr/src/gtest/build

# Copy your project files
COPY . .

# Create build directory and build the project
WORKDIR /workspace


# Set the entry point
CMD ["./build/metasim"]