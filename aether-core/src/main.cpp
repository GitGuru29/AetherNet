#include <iostream>
#include <vector>
#include "tun/tun_device.h"
#include "routing/ml_router.h"

int main() {
    std::cout << "Starting AetherNet Core Daemon..." << std::endl;
    
    aether::TunDevice tun("aether0");
    if (!tun.initialize()) {
        std::cerr << "Failed to initialize TUN interface. Exiting." << std::endl;
        return 1;
    }

    aether::MlRouter router;
    router.start_routing();

    std::cout << "Entering packet capture loop..." << std::endl;
    const int BUFFER_SIZE = 65535;
    char buffer[BUFFER_SIZE];

    // Main event loop capturing OS packets and routing them
    while (true) {
        int bytes_read = tun.read_packet(buffer, BUFFER_SIZE);
        
        if (bytes_read > 0) {
            // Push the raw OS packet through the ML routing and serialization logic
            std::string serialized_aether_packet = router.process_outgoing(buffer, bytes_read, "client-macos-local");
            
            // TODO: Push `serialized_aether_packet` over an encrypted TCP/UDP socket to Torch Proxy nodes
            
        } else {
            // For now, break if read returns error (e.g. fd closed or not blocking properly)
            // In a real implementation via epoll/kqueue, we wait for readiness
            break;
        }
    }

    return 0;
}
