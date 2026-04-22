#pragma once

// ============================================================
// Platform abstraction layer for cross-platform socket/network
// ============================================================

#ifdef _WIN32
    // --- Windows ---
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "Ws2_32.lib")

    using ssize_t = int;
    using socklen_t = int;

    inline int platform_close_socket(int fd) { return closesocket(fd); }

    // Initialize Winsock — must be called once at startup
    inline bool platform_socket_init() {
        WSADATA wsa;
        return WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
    }
    inline void platform_socket_cleanup() { WSACleanup(); }

#else
    // --- POSIX (macOS / Linux) ---
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <unistd.h>

    inline int platform_close_socket(int fd) { return close(fd); }
    inline bool platform_socket_init() { return true; }
    inline void platform_socket_cleanup() {}

#endif

// ============================================================
// Cross-platform IP/TCP/UDP header structures for DPI parsing
// ============================================================

#ifdef _WIN32
    // Windows doesn't have <netinet/ip.h> etc.
    // Define minimal structs for zero-copy DPI parsing.
    #include <cstdint>

    struct aether_ip_header {
    #if BYTE_ORDER == LITTLE_ENDIAN
        uint8_t  ihl:4, version:4;
    #else
        uint8_t  version:4, ihl:4;
    #endif
        uint8_t  tos;
        uint16_t tot_len;
        uint16_t id;
        uint16_t frag_off;
        uint8_t  ttl;
        uint8_t  protocol;
        uint16_t check;
        uint32_t saddr;
        uint32_t daddr;
    };

    struct aether_tcp_header {
        uint16_t source;
        uint16_t dest;
        uint32_t seq;
        uint32_t ack_seq;
    #if BYTE_ORDER == LITTLE_ENDIAN
        uint16_t res1:4, doff:4, fin:1, syn:1, rst:1, psh:1, ack:1, urg:1, ece:1, cwr:1;
    #else
        uint16_t doff:4, res1:4, cwr:1, ece:1, urg:1, ack:1, psh:1, rst:1, syn:1, fin:1;
    #endif
        uint16_t window;
        uint16_t check;
        uint16_t urg_ptr;
    };

    struct aether_udp_header {
        uint16_t source;
        uint16_t dest;
        uint16_t len;
        uint16_t check;
    };

    #define IPPROTO_TCP 6
    #define IPPROTO_UDP 17

#else
    // POSIX systems
    #include <netinet/in.h>
    #include <netinet/ip.h>
    #include <netinet/tcp.h>
    #include <netinet/udp.h>

    // Normalize to common struct names used in DPI code
    using aether_ip_header  = struct ip;
    using aether_tcp_header = struct tcphdr;
    using aether_udp_header = struct udphdr;

#endif
