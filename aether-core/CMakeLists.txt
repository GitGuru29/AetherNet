cmake_minimum_required(VERSION 3.15)
project(AetherCore CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)
include_directories(src)

add_executable(aether_daemon
    src/main.cpp
    src/tun/tun_device.cpp
    src/routing/ml_router.cpp
    src/transport/proxy_client.cpp
)

# target_link_libraries(aether_daemon protobuf)
