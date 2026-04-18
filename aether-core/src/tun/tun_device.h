#pragma once
#include <string>

namespace aether {

class TunDevice {
public:
    explicit TunDevice(const std::string& dev_name);
    ~TunDevice();

    bool initialize();
    int read_packet(char* buffer, int length);
    int write_packet(const char* buffer, int length);

private:
    std::string name;
    int fd;
};

} // namespace aether
