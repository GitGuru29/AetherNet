#include "ml_router.h"
#include <iostream>
#include <chrono>

// Note: Requires protoc to be run first to generate this header
#include "proto/packet.pb.h"
#include "proto/control.pb.h"

namespace aether {

MlRouter::MlRouter() {
    std::cout << "[MlRouter] Initializing ML Context (ONNX placeholder)..." << std::endl;
}

void MlRouter::start_routing() {
    std::cout << "[MlRouter] Context-aware routing engine activated." << std::endl;
}

uint64_t MlRouter::get_current_timestamp_us() {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(now).count();
}

uint32_t MlRouter::predict_traffic_class(const char* raw_packet, int length) {
    // Placeholder ML Heuristic
    // ML logic evaluates contextual hardware telemetry (Health, Latency)
    if (current_proxy_health < 0.5f || current_proxy_latency > 200.0f) {
        // Shift bandwidth-heavy operations to background class natively.
        if (length > 1000) return 3; // Demote streaming to background
    }
    
    if (length > 1000) return 1; // Likely streaming/bulk transport
    if (length < 100) return 2;  // Likely game telemetry or ACK
    return 3;
}

void MlRouter::process_telemetry(const char* telemetry_buffer, int length) {
    aether::proto::NodeTelemetry telemetry;
    if (telemetry.ParseFromArray(telemetry_buffer, length)) {
        current_proxy_health = telemetry.health_score();
        current_proxy_latency = telemetry.latency_ms();
        std::cout << "[ControlPlane] Proxy Node '" << telemetry.node_id() 
                  << "' -> Health: " << current_proxy_health 
                  << " | Latency: " << current_proxy_latency << "ms" << std::endl;
    } else {
        std::cerr << "[ControlPlane] Failed to parse NodeTelemetry!" << std::endl;
    }
}

std::string MlRouter::process_outgoing(const char* raw_packet, int length, const std::string& source_id) {
    // ==========================================
    // PROTOBUF SERIALIZATION LOGIC
    // ==========================================
    
    aether::proto::AetherPacket wrapper;
    wrapper.set_source_id(source_id);
    wrapper.set_target_id("auto-select-proxy-01");
    wrapper.set_sequence_num(1); // Would be tracked internally
    wrapper.set_timestamp_us(get_current_timestamp_us());
    wrapper.set_traffic_class(predict_traffic_class(raw_packet, length));
    wrapper.set_payload(raw_packet, length);
    
    std::string serialized_data;
    wrapper.SerializeToString(&serialized_data);
    
    std::cout << "[MlRouter] Origin: " << source_id << " | Encapsulated Packet (" << length 
              << " bytes) -> ML Predicted Traffic Class: " << predict_traffic_class(raw_packet, length) << std::endl;
              
    return serialized_data;
}

std::vector<char> MlRouter::process_incoming(const char* aether_buffer, int length) {
    // ==========================================
    // PROTOBUF DESERIALIZATION LOGIC
    // ==========================================
    
    aether::proto::AetherPacket wrapper;
    if (wrapper.ParseFromArray(aether_buffer, length)) {
        const std::string& raw = wrapper.payload();
        std::cout << "[MlRouter] Decapsulating incoming Aether packet. Payload size: " << raw.size() << " bytes." << std::endl;
        return std::vector<char>(raw.begin(), raw.end());
    }
    
    std::cerr << "[MlRouter] Failed to parse incoming Aether packet!" << std::endl;
    return std::vector<char>();
}

} // namespace aether
