#!/bin/bash
set -e

echo "Generating Protobuf bindings..."

# Ensure output directories exist
mkdir -p cpp_out
mkdir -p java_out

# Compile C++ and Java (Kotlin compatible) bindings
protoc -I=proto \
  --cpp_out=cpp_out \
  --java_out=java_out \
  proto/packet.proto proto/control.proto

echo "Bindings generated successfully!"
echo "C++ bindings located in aether-proto/cpp_out"
echo "Java bindings located in aether-proto/java_out"
