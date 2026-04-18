#include "proxy_client.h"
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

namespace aether {

ProxyClient::ProxyClient(const std::string& target_ip, int target_port)
    : ip(target_ip), port(target_port), sock_fd(-1) {
    memset(&server_addr, 0, sizeof(server_addr));
}

ProxyClient::~ProxyClient() {
    if (sock_fd >= 0) {
        close(sock_fd);
    }
}

bool ProxyClient::connect() {
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0); // Non-blocking UDP Socket
    if (sock_fd < 0) {
        std::cerr << "[ProxyClient] Failed to create UDP socket." << std::endl;
        return false;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    // Parse human readable IP address to binary routing format
    if (inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) <= 0) {
        std::cerr << "[ProxyClient] Invalid address format: " << ip << std::endl;
        return false;
    }

    std::cout << "[ProxyClient] UDP Transport initialized. Routing targeting -> " << ip << ":" << port << std::endl;
    return true;
}

int ProxyClient::send_data(const std::string& data) {
    if (sock_fd < 0) return -1;
    
    // Fire-and-forget UDP transmission for maximum throughput
    int bytes_sent = sendto(sock_fd, data.c_str(), data.length(), 0,
                            (const struct sockaddr*)&server_addr, sizeof(server_addr));
                            
    return bytes_sent;
}

} // namespace aether
