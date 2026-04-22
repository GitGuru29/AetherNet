#pragma once
#include <unordered_map>
#include <shared_mutex>
#include <cstdint>
#include <chrono>

namespace aether {

// Fast, padded 5-tuple identifying a unique network flow
struct FlowTuple {
    uint32_t src_ip;
    uint32_t dst_ip;
    uint16_t src_port;
    uint16_t dst_port;
    uint8_t  protocol;
    
    bool operator==(const FlowTuple& other) const {
        return src_ip == other.src_ip && dst_ip == other.dst_ip && 
               src_port == other.src_port && dst_port == other.dst_port && 
               protocol == other.protocol;
    }
};

} // namespace aether

// Custom hash function for std::unordered_map bridging
namespace std {
    template<> struct hash<aether::FlowTuple> {
        std::size_t operator()(const aether::FlowTuple& k) const {
            return ((hash<uint32_t>()(k.src_ip) ^ (hash<uint32_t>()(k.dst_ip) << 1)) >> 1) ^
                   (hash<uint16_t>()(k.src_port) << 1) ^ (hash<uint8_t>()(k.protocol) << 2);
        }
    };
}

namespace aether {

class Conntrack {
public:
    Conntrack();
    
    // Thread-safe map retrieval: returns session ID or allocates a new one
    int get_or_create_session(const FlowTuple& tuple);
    
    // Cleans up old connections that haven't been seen recently
    void expire_stale_sessions();

private:
   struct SessionEntry {
       int session_id;
       std::chrono::steady_clock::time_point last_seen;
   };

   std::unordered_map<FlowTuple, SessionEntry> session_table;
   std::shared_mutex map_mutex; // For Read-Writer lock performance
   int next_session_id;
};

} // namespace aether
