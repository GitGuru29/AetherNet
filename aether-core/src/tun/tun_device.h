#pragma once
#include <string>

#ifdef _WIN32
    #include <windows.h>
#endif

namespace aether {

class TunDevice {
public:
    explicit TunDevice(const std::string& dev_name);
    ~TunDevice();

    bool initialize();
    int read_packet(char* buffer, int length);
    int write_packet(const char* buffer, int length);
    
    std::string get_name() const { return name; }

private:
    std::string name;
#ifdef _WIN32
    HANDLE handle;  // Windows uses HANDLE for TAP device
#else
    int fd;
#endif
};

} // namespace aether
