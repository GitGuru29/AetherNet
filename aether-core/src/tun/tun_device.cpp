#include "tun_device.h"
#include <iostream>
#include <cstring>

#ifdef _WIN32
// ===================== WINDOWS TAP-WINDOWS6 IMPLEMENTATION =====================
#include <windows.h>
#include <winioctl.h>

// TAP-Windows6 adapter GUID (standard OpenVPN TAP driver)
#define TAP_WIN_IOCTL(request) CTL_CODE(FILE_DEVICE_UNKNOWN, request, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define TAP_WIN_IOCTL_SET_MEDIA_STATUS TAP_WIN_IOCTL(6)

// Registry path to enumerate TAP adapters
static const char* NETWORK_CONNECTIONS_KEY = 
    "SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}";
static const char* ADAPTER_KEY = 
    "SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}";

static std::string find_tap_adapter_guid() {
    // Enumerate network adapters in registry to find TAP-Windows6
    HKEY adapters_key;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, ADAPTER_KEY, 0, KEY_READ, &adapters_key) != ERROR_SUCCESS) {
        return "";
    }

    char enum_name[256];
    for (DWORD i = 0; RegEnumKeyA(adapters_key, i, enum_name, sizeof(enum_name)) == ERROR_SUCCESS; i++) {
        std::string subkey_path = std::string(ADAPTER_KEY) + "\\" + enum_name;
        HKEY subkey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, subkey_path.c_str(), 0, KEY_READ, &subkey) == ERROR_SUCCESS) {
            char component_id[256] = {0};
            DWORD component_len = sizeof(component_id);
            DWORD type;
            if (RegQueryValueExA(subkey, "ComponentId", NULL, &type, (LPBYTE)component_id, &component_len) == ERROR_SUCCESS) {
                if (strcmp(component_id, "tap0901") == 0 || strcmp(component_id, "root\\tap0901") == 0) {
                    // Found a TAP adapter — read its NetCfgInstanceId (GUID)
                    char guid[256] = {0};
                    DWORD guid_len = sizeof(guid);
                    if (RegQueryValueExA(subkey, "NetCfgInstanceId", NULL, &type, (LPBYTE)guid, &guid_len) == ERROR_SUCCESS) {
                        RegCloseKey(subkey);
                        RegCloseKey(adapters_key);
                        return std::string(guid);
                    }
                }
            }
            RegCloseKey(subkey);
        }
    }
    RegCloseKey(adapters_key);
    return "";
}

namespace aether {

TunDevice::TunDevice(const std::string& dev_name) : name(dev_name), handle(INVALID_HANDLE_VALUE) {}

TunDevice::~TunDevice() {
    if (handle != INVALID_HANDLE_VALUE) {
        CloseHandle(handle);
    }
}

bool TunDevice::initialize() {
    std::cout << "Initializing TUN device (Windows TAP-Windows6)..." << std::endl;

    std::string guid = find_tap_adapter_guid();
    if (guid.empty()) {
        std::cerr << "No TAP-Windows6 adapter found. Install OpenVPN TAP driver." << std::endl;
        return false;
    }

    std::string device_path = "\\\\.\\Global\\" + guid + ".tap";
    handle = CreateFileA(
        device_path.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_OVERLAPPED,
        NULL
    );

    if (handle == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to open TAP device: " << device_path << std::endl;
        return false;
    }

    // Bring the adapter up (set media status to connected)
    ULONG status = TRUE;
    DWORD bytes_returned;
    if (!DeviceIoControl(handle, TAP_WIN_IOCTL_SET_MEDIA_STATUS,
                         &status, sizeof(status), &status, sizeof(status),
                         &bytes_returned, NULL)) {
        std::cerr << "Failed to set TAP media status to connected." << std::endl;
        CloseHandle(handle);
        handle = INVALID_HANDLE_VALUE;
        return false;
    }

    name = "TAP:" + guid.substr(0, 8);
    std::cout << "Successfully bound to interface: " << name << std::endl;
    return true;
}

int TunDevice::read_packet(char* buffer, int length) {
    if (handle == INVALID_HANDLE_VALUE) return -1;
    DWORD bytes_read = 0;
    OVERLAPPED overlapped = {0};
    overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    
    if (!ReadFile(handle, buffer, length, &bytes_read, &overlapped)) {
        if (GetLastError() == ERROR_IO_PENDING) {
            WaitForSingleObject(overlapped.hEvent, INFINITE);
            GetOverlappedResult(handle, &overlapped, &bytes_read, FALSE);
        } else {
            CloseHandle(overlapped.hEvent);
            return -1;
        }
    }
    CloseHandle(overlapped.hEvent);
    return static_cast<int>(bytes_read);
}

int TunDevice::write_packet(const char* buffer, int length) {
    if (handle == INVALID_HANDLE_VALUE) return -1;
    DWORD bytes_written = 0;
    OVERLAPPED overlapped = {0};
    overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    
    if (!WriteFile(handle, buffer, length, &bytes_written, &overlapped)) {
        if (GetLastError() == ERROR_IO_PENDING) {
            WaitForSingleObject(overlapped.hEvent, INFINITE);
            GetOverlappedResult(handle, &overlapped, &bytes_written, FALSE);
        } else {
            CloseHandle(overlapped.hEvent);
            return -1;
        }
    }
    CloseHandle(overlapped.hEvent);
    return static_cast<int>(bytes_written);
}

} // namespace aether

#elif defined(__APPLE__)
// ===================== macOS UTUN IMPLEMENTATION =====================
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/kern_control.h>
#include <net/if_utun.h>
#include <sys/ioctl.h>
#include <sys/sys_domain.h>

namespace aether {

TunDevice::TunDevice(const std::string& dev_name) : name(dev_name), fd(-1) {}

TunDevice::~TunDevice() {
    if (fd >= 0) {
        close(fd);
    }
}

bool TunDevice::initialize() {
    std::cout << "Initializing TUN device (macOS utun)..." << std::endl;

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

#elif defined(__linux__)
// ===================== LINUX TUN IMPLEMENTATION =====================
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_tun.h>

namespace aether {

TunDevice::TunDevice(const std::string& dev_name) : name(dev_name), fd(-1) {}

TunDevice::~TunDevice() {
    if (fd >= 0) {
        close(fd);
    }
}

bool TunDevice::initialize() {
    std::cout << "Initializing TUN device (Linux /dev/net/tun)..." << std::endl;

    fd = open("/dev/net/tun", O_RDWR);
    if (fd < 0) {
        std::cerr << "Failed to open /dev/net/tun." << std::endl;
        return false;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI; // TUN device, no packet info overhead
    
    if (!name.empty()) {
        strncpy(ifr.ifr_name, name.c_str(), IFNAMSIZ);
    }

    if (ioctl(fd, TUNSETIFF, (void *)&ifr) < 0) {
        std::cerr << "Failed to TUNSETIFF ioctl." << std::endl;
        close(fd);
        return false;
    }

    name = std::string(ifr.ifr_name);

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

#else
#error "Unsupported platform for TUN device"
#endif
