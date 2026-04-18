#include <iostream>
#include "tun/tun_device.h"
#include "routing/ml_router.h"

int main() {
    std::cout << "Starting AetherNet Core Daemon..." << std::endl;
    
    aether::TunDevice tun("aether0");
    tun.initialize();

    aether::MlRouter router;
    router.start_routing();

    // Main event loop would go here
    return 0;
}
