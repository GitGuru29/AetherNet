FROM ubuntu:24.04

# Avoid timezone interactive prompt during installation
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies required by AetherNet Core
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libprotobuf-dev \
    protobuf-compiler \
    libabsl-dev \
    libcurl4-openssl-dev \
    iproute2 \
    iptables \
    iputils-ping \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy the entire workspace to get protos and core
COPY . /app/

# Build the AetherNet Core daemon
WORKDIR /app/aether-core
RUN rm -rf build && mkdir build && cd build && \
    cmake .. && \
    make -j$(nproc)

# Execute the daemon
CMD ["./build/aether_daemon"]
