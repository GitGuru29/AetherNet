#pragma once
#include <vector>
#include <cstring>
#include <random>

namespace aether {
namespace bench {

struct MockPacket {
    std::vector<char> data;
};

/**
 * Generates a pool of valid-looking network packets to exercise the DPI engine.
 * Packets are pre-allocated to avoid memory latency in the hot loop.
 */
class TrafficGenerator {
public:
    static std::vector<MockPacket> generate_pool(size_t pool_size) {
        std::vector<MockPacket> pool(pool_size);
        std::mt19937 gen(42);
        std::uniform_int_distribution<> d(64, 1500);

        for (size_t i = 0; i < pool_size; ++i) {
            int len = d(gen);
            pool[i].data.resize(len);
            
            // Minimal IP Header (20 bytes) starting at offset 0 (Linux)
            char* p = pool[i].data.data();
            p[0] = 0x45; // Version 4, IHL 5
            p[9] = (i % 2 == 0) ? 6 : 17; // Alternating TCP (6) and UDP (17)
            
            // Port numbers for DPI
            p[20] = 0x1f; p[21] = 0x90; // Source port 8080 (dummy)
        }
        return pool;
    }
};

} // namespace bench
} // namespace aether
