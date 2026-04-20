#include <iostream>
#include <vector>
#include <thread>
#include "tun/tun_device.h"
#include "routing/ml_router.h"
#include "transport/proxy_client.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
    std::cout << "Starting AetherNet Core Daemon..." << std::endl;
    
    aether::TunDevice tun("aether0");
    if (!tun.initialize()) {
        std::cerr << "Failed to initialize TUN interface. Exiting." << std::endl;
        return 1;
    }

    aether::MlRouter router;
    router.start_routing();

    // Boot up proxy transmission transport using Torch Labs integration
    std::string torch_api_key = "HACKATHON_DEMO_KEY"; // Replace with your actual key
    aether::ProxyClient proxy(torch_api_key);
    
    if (!proxy.fetch_torch_proxy_nodes()) {
        std::cerr << "Failed to fetch proxy nodes from Torch Labs API. Exiting." << std::endl;
        return 1;
    }
    
    if (!proxy.connect()) {
        std::cerr << "Failed to connect to Torch Proxy network. Exiting." << std::endl;
        return 1;
    }

    std::cout << "Entering multi-threaded packet routing engine..." << std::endl;

    // ----- THREAD 3: CONTROL PLANE ENGINE -----
    std::thread control_thread([&router]() {
        int ctrl_fd = socket(AF_INET, SOCK_DGRAM, 0);
        struct sockaddr_in ctrl_addr;
        ctrl_addr.sin_family = AF_INET;
        ctrl_addr.sin_port = htons(9002);
        ctrl_addr.sin_addr.s_addr = INADDR_ANY;
        
        if (bind(ctrl_fd, (struct sockaddr*)&ctrl_addr, sizeof(ctrl_addr)) < 0) {
            std::cerr << "[ControlPlane] Failed to bind to 9002" << std::endl;
            return;
        }
        std::cout << "[ControlPlane] Listening for NodeTelemetry on UDP port 9002..." << std::endl;
        
        char buffer[2048];
        while (true) {
            int bytes = recvfrom(ctrl_fd, buffer, sizeof(buffer), 0, nullptr, nullptr);
            if (bytes > 0) {
                router.process_telemetry(buffer, bytes);
            }
        }
    });
    control_thread.detach();

    // ----- THREAD 2: INBOUND ENGINE -----
    // Listens for UDP replies from the proxy and injects them back into the OS TUN interface
    std::thread inbound_thread([&tun, &router, &proxy]() {
        const int BUFFER_SIZE = 65535;
        char inbound_buffer[BUFFER_SIZE];
        
        while (true) {
            int bytes_received = proxy.receive_data(inbound_buffer, BUFFER_SIZE);
            if (bytes_received > 0) {
                // De-serialize the incoming Aether packet into a raw network frame
                std::vector<char> raw_ip_packet = router.process_incoming(inbound_buffer, bytes_received);
                
                // INJECT back into the Mac's networking stack
                int injected = tun.write_packet(raw_ip_packet.data(), raw_ip_packet.size());
                if (injected > 0) {
                    std::cout << "[Inbound Engine] Successfully injected " << injected 
                              << " byte response back to OS kernel!" << std::endl;
                }
            } else {
                std::cerr << "[Inbound Engine] Socket error or closed." << std::endl;
                break;
            }
        }
    });

    // ----- THREAD 1: OUTBOUND ENGINE (Main Thread) -----
    const int BUFFER_SIZE = 65535;
    char buffer[BUFFER_SIZE];

    // Main event loop capturing OS packets and routing them out
    while (true) {
        int bytes_read = tun.read_packet(buffer, BUFFER_SIZE);
        
        if (bytes_read > 0) {
            // ML-driven node health monitoring
            if (router.requires_hotswap()) {
                std::cout << "[Core] ML Router predicts node congestion/failure! Initiating Torch Proxy hotswap..." << std::endl;
                proxy.hotswap_proxy();
                router.reset_health();
            }

            std::string serialized_aether_packet = router.process_outgoing(buffer, bytes_read, "client-macos-local");
            
            int sent = proxy.send_data(serialized_aether_packet);
            if (sent < 0) {
                std::cerr << "[Warning] Dropped packet frame on transport layer!" << std::endl;
            }
            
        } else {
            break;
        }
    }

    // Wait for the inbound thread to finish (which it shouldn't unless error occurs)
    inbound_thread.join();

    return 0;
}
