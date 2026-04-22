#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace aether {

class MlRouter {
public:
    MlRouter();
    void start_routing();
    
    // Zero-Copy DPI packet inspection
    void inspect_packet(const char* buffer, int length);
    
    // Wraps raw IP packet from TUN into AetherPacket and routes it
    std::string process_outgoing(const char* raw_packet, int length, const std::string& source_id);
    
    // Decodes AetherPacket received from proxy and extracts raw IP packet
    std::vector<char> process_incoming(const char* aether_buffer, int length);

    // Decodes NodeTelemetry and updates internal proxy health metrics
    void process_telemetry(const char* telemetry_buffer, int length);

private:
    float current_proxy_health = 1.0f;
    float current_proxy_latency = 0.0f;
    uint64_t get_current_timestamp_us();
    uint32_t predict_traffic_class(const char* raw_packet, int length);

public:
    bool requires_hotswap() const {
        return current_proxy_health < 0.3f || current_proxy_latency > 400.0f;
    }
    
    void reset_health() {
        current_proxy_health = 1.0f;
        current_proxy_latency = 0.0f;
    }
};

} // namespace aether
