#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <atomic>

namespace aether {

class MlRouter {
public:
    MlRouter();
    void start_routing();
    
    // Zero-Copy DPI packet inspection
    void inspect_packet(const char* buffer, int length);
    void batch_inspect_packets(const char** buffers, const int* lengths, int batch_size);
    
private:
    void inspect_packet_internal(const char* buffer, int length);

public:
    // Wraps raw IP packet from TUN into AetherPacket and routes it
    std::string process_outgoing(const char* raw_packet, int length, const std::string& source_id);
    
    // Decodes AetherPacket received from proxy and extracts raw IP packet
    std::vector<char> process_incoming(const char* aether_buffer, int length);

    // Decodes NodeTelemetry and updates internal proxy health metrics
    void process_telemetry(const char* telemetry_buffer, int length);

private:
    std::atomic<float> current_proxy_health{1.0f};
    std::atomic<float> current_proxy_latency{0.0f};
    uint64_t get_current_timestamp_us();
    uint32_t predict_traffic_class(const char* raw_packet, int length);
    
    // Security tracking
    uint64_t last_telemetry_ts = 0;
    uint64_t max_seen_sequence = 0;
    std::string control_plane_secret = "DEMO_SECRET_K3Y"; // Use env var in production

    // Adversarial Stability Filter logic
    float apply_stability_filter(float target_health, float current_health);

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
