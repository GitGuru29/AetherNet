#include "tun_device.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>

#ifdef __APPLE__
#include <sys/socket.h>
#include <sys/kern_control.h>
#include <net/if_utun.h>
#include <sys/ioctl.h>
#include <sys/sys_domain.h>
#elif __linux__
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_tun.h>
#endif

namespace aether {

TunDevice::TunDevice(const std::string& dev_name) : name(dev_name), fd(-1) {}

TunDevice::~TunDevice() {
    if (fd >= 0) {
        close(fd);
    }
}

bool TunDevice::initialize() {
    std::cout << "Initializing TUN device..." << std::endl;

#ifdef __APPLE__
    fd = socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);
    if (fd < 0) {
        std::cerr << "Failed to create PF_SYSTEM socket." << std::endl;
        return false;
    }

    struct ctl_info info;
    memset(&info, 0, sizeof(info));
    strncpy(info.ctl_name, UTUN_CONTROL_NAME, sizeof(info.ctl_name));

    if (ioctl(fd, CTLIOCGINFO, &info) == -1) {
        std::cerr << "Failed to get utun control info." << std::endl;
        close(fd);
        return false;
    }

    struct sockaddr_ctl addr;
    memset(&addr, 0, sizeof(addr));
    addr.sc_id = info.ctl_id;
    addr.sc_len = sizeof(addr);
    addr.sc_family = AF_SYS_CONTROL;
    addr.ss_sysaddr = AF_SYS_CONTROL;
    addr.sc_unit = 0; // Let the OS assign dynamically

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        std::cerr << "Failed to connect to utun control." << std::endl;
        close(fd);
        return false;
    }

    char ifname[20];
    socklen_t ifname_len = sizeof(ifname);
    if (getsockopt(fd, SYSPROTO_CONTROL, UTUN_OPT_IFNAME, ifname, &ifname_len) == 0) {
        name = std::string(ifname);
    } else {
        name = "utun?"; // unknown
    }

#elif __linux__
    fd = open("/dev/net/tun", O_RDWR);
    if (fd < 0) {
        std::cerr << "Failed to open /dev/net/tun." << std::endl;
        return false;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI; // TUN device, no packet info overhead
    
    if (!name.empty() && name != "aether0") {
        strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ);
    }

    if (ioctl(fd, TUNSETIFF, (void *)&ifr) < 0) {
        std::cerr << "Failed to TUNSETIFF ioctl." << std::endl;
        close(fd);
        return false;
    }

    name = std::string(ifr.ifr_name);
#endif

    std::cout << "Successfully bound to interface: " << name << std::endl;
    return true;
}

int TunDevice::read_packet(char* buffer, int length) {
    if (fd < 0) return -1;
    return read(fd, buffer, length);
}

int TunDevice::write_packet(const char* buffer, int length) {
    if (fd < 0) return -1;
    return write(fd, buffer, length);
}

} // namespace aether
