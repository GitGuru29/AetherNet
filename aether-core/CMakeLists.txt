cmake_minimum_required(VERSION 3.15)
project(AetherCore CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Protobuf REQUIRED)
find_package(absl REQUIRED)
find_package(CURL REQUIRED)

# --- Security & Sanitizer Options ---
option(USE_SANITIZERS "Enable Address and Undefined Behavior Sanitizers" OFF)
option(BUILD_FUZZERS "Build fuzzing harnesses (requires libFuzzer)" OFF)

if(USE_SANITIZERS)
    message(STATUS "Integrating AddressSanitizer and UndefinedBehaviorSanitizer...")
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

include_directories(${Protobuf_INCLUDE_DIRS})
include_directories(include)
include_directories(src)

add_executable(aether_daemon
    src/main.cpp
    src/tun/tun_device.cpp
    src/routing/ml_router.cpp
    src/routing/conntrack.cpp
    src/transport/proxy_client.cpp
    src/proto/packet.pb.cc
    src/proto/control.pb.cc
)

target_link_libraries(aether_daemon protobuf::libprotobuf absl::log_internal_message absl::log_internal_check_op CURL::libcurl)

# --- Performance Benchmarking ---
add_executable(aether_perf_bench 
    tests/bench/aether_perf_bench.cpp 
    src/routing/ml_router.cpp 
    src/proto/packet.pb.cc
)
target_compile_options(aether_perf_bench PRIVATE -march=native -O3 -fopenmp-simd)
target_link_libraries(aether_perf_bench protobuf::libprotobuf absl::log_internal_message)

# --- Fuzzing Targets ---
if(BUILD_FUZZERS)
    message(STATUS "Configuring Fuzzing targets...")
    find_library(LIBPROTOBUF_MUTATOR NAMES protobuf-mutator REQUIRED)
    
    add_executable(fuzz_packet tests/fuzz/packet_fuzzer.cpp src/routing/ml_router.cpp src/proto/packet.pb.cc)
    target_compile_options(fuzz_packet PRIVATE -fsanitize=fuzzer)
    target_link_libraries(fuzz_packet protobuf::libprotobuf ${LIBPROTOBUF_MUTATOR} -fsanitize=fuzzer)
endif()
