#pragma once
#include <thread>
#include <iostream>

#ifdef __linux__
#include <pthread.h>
#include <sched.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

namespace aether {
namespace utils {

/**
 * Pins the calling thread to a specific CPU core to minimize context switching
 * and cache misses in high-performance networking loops.
 */
inline bool pin_thread_to_core(int core_id) {
#ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    pthread_t current_thread = pthread_self();
    if (pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset) != 0) {
        std::cerr << "[Perf] Failed to pin thread to core " << core_id << std::endl;
        return false;
    }
#elif defined(_WIN32)
    HANDLE thread = GetCurrentThread();
    DWORD_PTR mask = 1ULL << core_id;
    if (SetThreadAffinityMask(thread, mask) == 0) {
        std::cerr << "[Perf] Failed to pin thread to core " << core_id << std::endl;
        return false;
    }
#endif
    std::cout << "[Perf] Successfully pinned thread to core " << core_id << std::endl;
    return true;
}

} // namespace utils
} // namespace aether
