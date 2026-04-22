#pragma once
#include <x86intrin.h>
#include <cstdint>
#include <algorithm>
#include <vector>

namespace aether {
namespace bench {

/**
 * High-resolution cycle counting using TSC (Time Stamp Counter).
 * Uses LFENCE for execution serialization to prevent out-of-order skew.
 */
inline uint64_t read_tsc_start() {
    _mm_lfence();
    return __rdtsc();
}

inline uint64_t read_tsc_end() {
    unsigned int aux;
    uint64_t tsc = __rdtscp(&aux);
    _mm_lfence();
    return tsc;
}

/**
 * Statistics container for benchmark results.
 */
struct BenchStats {
    uint64_t total_packets;
    uint64_t total_cycles;
    double avg_cpp;
    std::vector<uint64_t> samples;

    void calculate() {
        if (samples.empty()) return;
        std::sort(samples.begin(), samples.end());
        avg_cpp = static_cast<double>(total_cycles) / total_packets;
    }

    uint64_t p50() const { return samples[samples.size() * 0.50]; }
    uint64_t p90() const { return samples[samples.size() * 0.90]; }
    uint64_t p99() const { return samples[samples.size() * 0.99]; }
};

} // namespace bench
} // namespace aether
