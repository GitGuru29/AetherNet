#include "ml_router.h"
#include <iostream>
#include <chrono>

// Cross-platform network headers + unified struct aliases
#include "platform/platform.h"

// Note: Requires protoc to be run first to generate this header
#include "proto/packet.pb.h"
#include "proto/control.pb.h"

namespace aether {

// ============================================================
// DPI packet field accessors — normalise across OS / struct layouts
// ============================================================
namespace dpi {

// ---- IP header ----
#ifdef _WIN32
    inline uint8_t  ip_hlen(const aether_ip_header* h) { return h->ihl * 4; }
    inline uint8_t  ip_proto(const aether_ip_header* h) { return h->protocol; }
#elif defined(__APPLE__)
    inline uint8_t  ip_hlen(const aether_ip_header* h) { return h->ip_hl * 4; }
    inline uint8_t  ip_proto(const aether_ip_header* h) { return h->ip_p; }
#else // Linux
    inline uint8_t  ip_hlen(const aether_ip_header* h) { return h->ihl * 4; }
    inline uint8_t  ip_proto(const aether_ip_header* h) { return h->protocol; }
#endif

// ---- TCP header ----
#ifdef _WIN32
    inline uint16_t tcp_sport(const aether_tcp_header* h) { return ntohs(h->source); }
    inline uint16_t tcp_dport(const aether_tcp_header* h) { return ntohs(h->dest); }
    inline bool     tcp_syn  (const aether_tcp_header* h) { return h->syn; }
    inline bool     tcp_fin  (const aether_tcp_header* h) { return h->fin; }
#elif defined(__APPLE__)
    inline uint16_t tcp_sport(const aether_tcp_header* h) { return ntohs(h->th_sport); }
    inline uint16_t tcp_dport(const aether_tcp_header* h) { return ntohs(h->th_dport); }
    inline bool     tcp_syn  (const aether_tcp_header* h) { return h->th_flags & TH_SYN; }
    inline bool     tcp_fin  (const aether_tcp_header* h) { return h->th_flags & TH_FIN; }
#else // Linux
    inline uint16_t tcp_sport(const aether_tcp_header* h) { return ntohs(h->source); }
    inline uint16_t tcp_dport(const aether_tcp_header* h) { return ntohs(h->dest); }
    inline bool     tcp_syn  (const aether_tcp_header* h) { return h->syn; }
    inline bool     tcp_fin  (const aether_tcp_header* h) { return h->fin; }
#endif

// ---- UDP header ----
#ifdef _WIN32
    inline uint16_t udp_sport(const aether_udp_header* h) { return ntohs(h->source); }
    inline uint16_t udp_dport(const aether_udp_header* h) { return ntohs(h->dest); }
#elif defined(__APPLE__)
    inline uint16_t udp_sport(const aether_udp_header* h) { return ntohs(h->uh_sport); }
    inline uint16_t udp_dport(const aether_udp_header* h) { return ntohs(h->uh_dport); }
#else // Linux
    inline uint16_t udp_sport(const aether_udp_header* h) { return ntohs(h->source); }
    inline uint16_t udp_dport(const aether_udp_header* h) { return ntohs(h->dest); }
#endif

} // namespace dpi

// ============================================================
// TUN packet offsets
// On macOS/utun the kernel prepends a 4-byte address-family header
// (AF_INET = 0x00000002 in network order). On Linux IFF_NO_PI strips
// the extra packet-info header, so raw IP starts at byte 0.
// On Windows the TAP driver hands us full Ethernet frames — but for
// simplicity we also skip no bytes (TUN mode strips the ETH header).
// ============================================================
static constexpr int TUN_OFFSET =
#ifdef __APPLE__
    4;  // utun prepends 4-byte PI
#else
    0;  // Linux TUN (IFF_NO_PI) and Windows TAP (TUN mode)
#endif

// ============================================================

void MlRouter::inspect_packet(const char* buffer, int length) {
    const int offset = TUN_OFFSET;

    if (length < offset + static_cast<int>(sizeof(aether_ip_header))) return;

    // Zero-copy cast directly into buffer
    const auto* ip_hdr = reinterpret_cast<const aether_ip_header*>(buffer + offset);
    int ip_hdr_len = dpi::ip_hlen(ip_hdr);
    if (length < offset + ip_hdr_len) return;

    uint8_t  protocol  = dpi::ip_proto(ip_hdr);
    uint16_t src_port  = 0;
    uint16_t dst_port  = 0;

    if (protocol == IPPROTO_TCP) {
        if (length < offset + ip_hdr_len + static_cast<int>(sizeof(aether_tcp_header))) return;
        const auto* tcp_hdr = reinterpret_cast<const aether_tcp_header*>(
            buffer + offset + ip_hdr_len);

        src_port = dpi::tcp_sport(tcp_hdr);
        dst_port = dpi::tcp_dport(tcp_hdr);
        bool syn = dpi::tcp_syn(tcp_hdr);
        bool fin = dpi::tcp_fin(tcp_hdr);

        std::cout << "[DPI] TCP | Src: " << src_port << " | Dst: " << dst_port
                  << " | SYN: " << syn << " | FIN: " << fin << std::endl;

    } else if (protocol == IPPROTO_UDP) {
        if (length < offset + ip_hdr_len + static_cast<int>(sizeof(aether_udp_header))) return;
        const auto* udp_hdr = reinterpret_cast<const aether_udp_header*>(
            buffer + offset + ip_hdr_len);

        src_port = dpi::udp_sport(udp_hdr);
        dst_port = dpi::udp_dport(udp_hdr);

        std::cout << "[DPI] UDP | Src: " << src_port << " | Dst: " << dst_port << std::endl;

    } else {
        std::cout << "[DPI] Protocol ID: " << static_cast<int>(protocol) << std::endl;
    }
}

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
    if (current_proxy_health < 0.5f || current_proxy_latency > 200.0f) {
        if (length > 1000) return 3;
    }
    if (length > 1000) return 1;
    if (length < 100)  return 2;
    return 3;
}

void MlRouter::process_telemetry(const char* telemetry_buffer, int length) {
    aether::proto::NodeTelemetry telemetry;
    if (telemetry.ParseFromArray(telemetry_buffer, length)) {
        current_proxy_health  = telemetry.health_score();
        current_proxy_latency = telemetry.latency_ms();
        std::cout << "[ControlPlane] Proxy Node '" << telemetry.node_id()
                  << "' -> Health: " << current_proxy_health
                  << " | Latency: "  << current_proxy_latency << "ms" << std::endl;
    } else {
        std::cerr << "[ControlPlane] Failed to parse NodeTelemetry!" << std::endl;
    }
}

std::string MlRouter::process_outgoing(const char* raw_packet, int length,
                                       const std::string& source_id) {
    aether::proto::AetherPacket wrapper;
    wrapper.set_source_id(source_id);
    wrapper.set_target_id("auto-select-proxy-01");
    wrapper.set_sequence_num(1);
    wrapper.set_timestamp_us(get_current_timestamp_us());
    wrapper.set_traffic_class(predict_traffic_class(raw_packet, length));
    wrapper.set_payload(raw_packet, length);
    
    std::string serialized_data;
    if (!wrapper.SerializeToString(&serialized_data)) {
        std::cerr << "[MlRouter] Failed to serialize packet!" << std::endl;
    }
    std::cout << "[MlRouter] Origin: " << source_id
              << " | Encapsulated Packet (" << length
              << " bytes) -> ML Predicted Traffic Class: "
              << predict_traffic_class(raw_packet, length) << std::endl;

    return serialized_data;
}

std::vector<char> MlRouter::process_incoming(const char* aether_buffer, int length) {
    aether::proto::AetherPacket wrapper;
    if (wrapper.ParseFromArray(aether_buffer, length)) {
        const std::string& raw = wrapper.payload();
        std::cout << "[MlRouter] Decapsulating incoming Aether packet. Payload size: "
                  << raw.size() << " bytes." << std::endl;
        return std::vector<char>(raw.begin(), raw.end());
    }
    
    std::cerr << "[MlRouter] Failed to parse incoming Aether packet!" << std::endl;
    return std::vector<char>();
}

} // namespace aether
