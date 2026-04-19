cmake_minimum_required(VERSION 3.15)
project(AetherCore CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Protobuf REQUIRED)
find_package(absl REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})

include_directories(include)
include_directories(src)

add_executable(aether_daemon
    src/main.cpp
    src/tun/tun_device.cpp
    src/routing/ml_router.cpp
    src/transport/proxy_client.cpp
    src/proto/packet.pb.cc
    src/proto/control.pb.cc
)

target_link_libraries(aether_daemon protobuf::libprotobuf absl::log_internal_message absl::log_internal_check_op)
