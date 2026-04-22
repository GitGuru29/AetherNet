#pragma once
#include <string>
#include <vector>

// Use our platform abstraction instead of POSIX-only <netinet/in.h>
#include "platform/platform.h"

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <netinet/in.h>
#endif

namespace aether {

class ProxyClient {
public:
    ProxyClient(const std::string& api_key);
    ~ProxyClient();

    bool fetch_torch_proxy_nodes();
    bool connect();
    void hotswap_proxy(); // Switch to another proxy if congested

    int send_data(const std::string& data);
    int receive_data(char* buffer, int max_length);
    
    std::string get_current_proxy_ip() const { return current_ip; }

private:
    std::string api_key;
    std::vector<std::pair<std::string, int>> proxy_pool;
    int current_proxy_index;
    
    std::string current_ip;
    int current_port;
    
    // SOCKET is UINT_PTR on Windows, int on POSIX — use int with a cast to SOCKET on Windows
    int sock_fd;
    struct sockaddr_in server_addr;
};

} // namespace aether
