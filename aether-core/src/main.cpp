#include <iostream>
#include <vector>
#include <thread>
#include "tun/tun_device.h"
#include "routing/ml_router.h"
#include "transport/proxy_client.h"

int main() {
    std::cout << "Starting AetherNet Core Daemon..." << std::endl;
    
    aether::TunDevice tun("aether0");
    if (!tun.initialize()) {
        std::cerr << "Failed to initialize TUN interface. Exiting." << std::endl;
        return 1;
    }

    aether::MlRouter router;
    router.start_routing();

    // Boot up proxy transmission transport
    aether::ProxyClient proxy("127.0.0.1", 9000);
    if (!proxy.connect()) {
        std::cerr << "Failed to boot UDP transport client. Exiting." << std::endl;
        return 1;
    }

    std::cout << "Entering dual-threaded packet routing engine..." << std::endl;

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
