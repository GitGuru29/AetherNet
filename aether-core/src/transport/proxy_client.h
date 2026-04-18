#pragma once
#include <string>
#include <netinet/in.h>

namespace aether {

class ProxyClient {
public:
    ProxyClient(const std::string& target_ip, int target_port);
    ~ProxyClient();

    bool connect();
    int send_data(const std::string& data);
    int receive_data(char* buffer, int max_length);

private:
    std::string ip;
    int port;
    int sock_fd;
    struct sockaddr_in server_addr;
};

} // namespace aether
