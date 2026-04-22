# AetherNet: High-Performance Hardened Network Daemon

AetherNet is an enterprise-grade, multi-threaded C++ network daemon designed for high-assurance protocol routing. It features a modern DevSecOps pipeline, 10Gbps+ kernel-bypass architecture, and an ML-driven adversarial-aware routing engine.

---

## 🚀 Key Features

### 🛡️ Hardened Security
- **Control Plane Authentication**: HMAC-SHA256 signing for all telemetry heartbeats.
- **Data Plane Resilience**: 64-bit sequence numbering for complete replay protection.
- **Adversarial Stability Filter**: ML-logic that dampens sudden telemetry drops to prevent malicious routing hotswaps.
- **Deep Packet Inspection (DPI)**: Zero-copy header inspection with proactive IHL validation and OOB read protection.

### ⚡ 10Gbps+ High-Performance Scaling
- **AF_XDP Kernel Bypass**: Zero-copy packet I/O using Linux UMEM memory pools.
- **Vectorized DPI**: Batch processing (32/64 packets) leveraging **AVX-512** SIMD intrinsics.
- **Lock-Free Architecture**: C++20 atomic-float metrics eliminate mutex contention.
- **Hardware-Awareness**: Automatic thread pinning (affinity) and Hugepage (1GB/2MB) memory support.

### 🛠️ DevSecOps Pipeline
- **Continuous Fuzzing**: Integrated `libprotobuf-mutator` for deep protocol fuzz testing.
- **Static Analysis**: Clang-Tidy (bugprone, cert, cert) and Cppcheck security scans.
- **Dynamic Analysis**: Runtime Address (ASan) and Undefined Behavior (UBSan) sanitizers.
- **Security Validation**: DAST layer using **Scapy** for protocol enforcement testing.

---

## 🏗️ Architecture Overview

The system is split into three high-performance execution cores:
1.  **Outbound Engine (Core 0)**: High-speed packet ingestion, DPI, and proxy encapsulation.
2.  **Inbound Engine (Core 2)**: De-encapsulation and injection into the OS networking stack.
3.  **Control Plane (Core 1)**: Authenticated UDP telemetry receptor for the ML-driven hotswap logic.

---

## 🔨 Build & Installation

### Prerequisites
- Linux Kernel ≥ 5.4 (for AF_XDP)
- Protobuf 3.15+
- Abseil-cpp, CURL
- Clang 12+ (for AVX-512 support)

### Compilation
```bash
# Clone the repository
git clone https://github.com/GitGuru29/AetherNet.git
cd AetherNet

# Standard Build
cmake -B build -DUSE_SANITIZERS=OFF -DBUILD_FUZZERS=OFF
cmake --build build -- -j$(nproc)

# High-Performance Build (AVX-512 Enabled)
cmake -B build -DCMAKE_CXX_FLAGS="-march=native"
```

---

## 📊 Benchmarking & Performance

To verify the Cycles-Per-Packet (CPP) and throughput (Mpps):
```bash
cmake --build build --target aether_perf_bench
./build/aether_perf_bench
```
The suite measures absolute execution cycles using hardware `rdtsc` counters.

---

## 🛡️ Security Testing

To run the automated security suite locally:
```bash
# Run SAST Scan
bash scripts/cppcheck_scan.sh

# Run Protocol Validation (Requires Root for Scapy)
sudo python3 scripts/security/scapy_protocol_test.py
```

---

