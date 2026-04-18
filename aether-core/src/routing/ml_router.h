#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace aether {

class MlRouter {
public:
    MlRouter();
    void start_routing();
    
    // Wraps raw IP packet from TUN into AetherPacket and routes it
    std::string process_outgoing(const char* raw_packet, int length, const std::string& source_id);
    
    // Decodes AetherPacket received from proxy and extracts raw IP packet
    std::vector<char> process_incoming(const char* aether_buffer, int length);

private:
    uint64_t get_current_timestamp_us();
    uint32_t predict_traffic_class(const char* raw_packet, int length);
};

} // namespace aether
