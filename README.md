# AetherNet

A High-Performance User-Space Network Stack designed to intercept raw network packets at the Data Link/Network Layer and process them through a custom virtual network interface (TUN/TAP). This project explores system-level network virtualization and context-aware traffic routing.

## Project Structure
- **`aether-core/`**: C++ System daemon managing TUN, serialization, and proxy integration.
- **`aether-proto/`**: Shared Protobuf definitions for custom wire protocols.
- **`aether-android-client/`**: Kotlin/Jetpack Compose system-level VPN client for Android.
- **`aether-monitor/`**: Rust/Svelte dashboard for visualizing global traffic routing.
