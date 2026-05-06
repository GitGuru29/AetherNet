#include <iostream>
#include <vector>
#include <iomanip>
#include "routing/ml_router.h"
#include "utils/cycle_counter.h"
#include "utils/performance_utils.h"
#include "tests/bench/traffic_gen.h"

using namespace aether::bench;

/**
 * AetherNet Performance Benchmark Utility
 * Measures Cycles-Per-Packet (CPP) for the vectorized DPI engine.
 */
int main(int argc, char** argv) {
    std::cout << "--- AetherNet Performance Benchmark ---" << std::endl;

    // 1. PIN to the last physical core for stable results
    aether::utils::pin_thread_to_core(3);

    // 2. Prepare Traffic Pool (64K packets)
    constexpr size_t POOL_SIZE = 65536;
    auto pool = TrafficGenerator::generate_pool(POOL_SIZE);
    
    std::vector<const char*> buffers(POOL_SIZE);
    std::vector<int> lengths(POOL_SIZE);
    for (size_t i = 0; i < POOL_SIZE; ++i) {
        buffers[i] = pool[i].data.data();
        lengths[i] = pool[i].data.size();
    }

    aether::MlRouter router;
    BenchStats stats;
    stats.total_packets = 0;
    stats.total_cycles = 0;

    // 3. Warm-up phase (Prime caches)
    router.batch_inspect_packets(buffers.data(), lengths.data(), 1000);

    // 4. MAIN BENCHMARK LOOP
    constexpr int ITERATIONS = 10000; // Total packets = 10000 * 64 = 640,000
    constexpr int BATCH_SIZE = 64;

    std::cout << "[Bench] Running " << ITERATIONS << " batches of size " << BATCH_SIZE << "..." << std::endl;

    for (int i = 0; i < ITERATIONS; ++i) {
        int offset = (i * BATCH_SIZE) % (POOL_SIZE - BATCH_SIZE);
        
        uint64_t start = read_tsc_start();
        router.batch_inspect_packets(buffers.data() + offset, lengths.data() + offset, BATCH_SIZE);
        uint64_t end = read_tsc_end();

        uint64_t batch_cycles = end - start;
        stats.total_cycles += batch_cycles;
        stats.total_packets += BATCH_SIZE;
        stats.samples.push_back(batch_cycles / BATCH_SIZE);
    }

    stats.calculate();

    // 5. Results Reporting
    std::cout << "\n--- Benchmark Results ---" << std::endl;
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Total Packets Processed : " << stats.total_packets << std::endl;
    std::cout << "Average Cycles/Packet   : " << stats.avg_cpp << std::endl;
    std::cout << "P50 Latency (Cycles)   : " << stats.p50() << std::endl;
    std::cout << "P99 Latency (Cycles)   : " << stats.p99() << std::endl;
    
    // Assuming 3.0GHz base clock for Mpps calculation example
    double freq_ghz = 3.0;
    double mpps = (freq_ghz * 1000.0) / stats.avg_cpp;
    std::cout << "Estimated Throughput    : " << mpps << " Mpps (@" << freq_ghz << "GHz core)" << std::endl;

    return 0;
}
