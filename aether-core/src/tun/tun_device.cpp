#include "tun_device.h"
#include <iostream>

namespace aether {

TunDevice::TunDevice(const std::string& dev_name) : name(dev_name), fd(-1) {}

TunDevice::~TunDevice() {
    // Cleanup FD
}

bool TunDevice::initialize() {
    std::cout << "Initializing TUN device: " << name << std::endl;
    // Platform specific TUN setup goes here
    return true;
}

int TunDevice::read_packet(char* buffer, int length) {
    // Read from TUN fd
    return 0; 
}

int TunDevice::write_packet(const char* buffer, int length) {
    // Write to TUN fd
    return 0;
}

} // namespace aether
