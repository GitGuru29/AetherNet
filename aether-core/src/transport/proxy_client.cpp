#include "proxy_client.h"
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <curl/curl.h>

namespace aether {

// libcurl write callback
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

ProxyClient::ProxyClient(const std::string& api_key)
    : api_key(api_key), current_proxy_index(-1), sock_fd(-1) {
    memset(&server_addr, 0, sizeof(server_addr));
}

ProxyClient::~ProxyClient() {
    if (sock_fd >= 0) close(sock_fd);
}

bool ProxyClient::fetch_torch_proxy_nodes() {
    CURL* curl = curl_easy_init();
    if (!curl) return false;
    
    std::string response_string;
    std::string url = "https://api.torchproxies.com/v1/nodes?key=" + api_key;
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
    
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        std::cerr << "[ProxyClient] Failed to fetch Torch Proxies: " << curl_easy_strerror(res) << std::endl;
        return false;
    }
    
    // MOCK PARSING: In a real scenario, use a JSON library like nlohmann/json
    // For now we populate dummy nodes to simulate the response
    proxy_pool.clear();
    proxy_pool.push_back({"192.168.1.10", 1080});
    proxy_pool.push_back({"192.168.1.11", 1080});
    proxy_pool.push_back({"192.168.1.12", 1080});
    
    std::cout << "[ProxyClient] Fetched " << proxy_pool.size() << " Torch Proxy nodes!" << std::endl;
    return !proxy_pool.empty();
}

void ProxyClient::hotswap_proxy() {
    if (proxy_pool.empty()) return;
    
    current_proxy_index = (current_proxy_index + 1) % proxy_pool.size();
    current_ip = proxy_pool[current_proxy_index].first;
    current_port = proxy_pool[current_proxy_index].second;
    
    std::cout << "[ProxyClient] Hotswapping to Torch Proxy node -> " << current_ip << ":" << current_port << std::endl;
    connect();
}

bool ProxyClient::connect() {
    if (current_ip.empty()) hotswap_proxy();
    
    if (sock_fd >= 0) close(sock_fd);
    
    sock_fd = socket(AF_INET, SOCK_STREAM, 0); // TCP for HTTP/SOCKS5 proxy
    if (sock_fd < 0) return false;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(current_port);
    inet_pton(AF_INET, current_ip.c_str(), &server_addr.sin_addr);

    // Normally here we would perform SOCKS5 handshake or HTTP CONNECT
    if (::connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "[ProxyClient] Failed to connect to Torch Proxy at " << current_ip << std::endl;
        return false;
    }

    std::cout << "[ProxyClient] Connected to Torch Proxy -> " << current_ip << ":" << current_port << std::endl;
    return true;
}

int ProxyClient::send_data(const std::string& data) {
    if (sock_fd < 0) return -1;
    return send(sock_fd, data.c_str(), data.length(), 0);
}

int ProxyClient::receive_data(char* buffer, int max_length) {
    if (sock_fd < 0) return -1;
    return recv(sock_fd, buffer, max_length, 0);
}

} // namespace aether
