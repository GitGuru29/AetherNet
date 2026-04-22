import sys
from scapy.all import IP, UDP, Raw, send
import time

# AetherNet Protocol Enforcement & Validation Script
# This script simulates various network-level attacks to verify daemon resilience.

TARGET_IP = "127.0.0.1"
CONTROL_PORT = 9002

def test_telemetry_flood():
    print(f"[*] Starting UDP Telemetry Flood against port {CONTROL_PORT}...")
    # Send malformed UDP packets to the control plane
    for i in range(100):
        # Craft a packet that is definitely NOT a valid Protobuf NodeTelemetry
        payload = b"\xff\x00\xde\xad\xbe\xef" * 50
        pkt = IP(dst=TARGET_IP)/UDP(sport=12345, dport=CONTROL_PORT)/Raw(load=payload)
        send(pkt, verbose=False)
    print("[+] Flood completed. Check daemon logs for 'Failed to parse NodeTelemetry'.")

def test_tls_downgrade_sim():
    print("[*] Simulating TLS Downgrade/Insecure Handshake Attempt...")
    # Attempt to send a packet with a header that looks like an old/insecure protocol
    insecure_payload = b"\x16\x03\x01\x00" # TLS 1.0 Client Hello signature
    pkt = IP(dst=TARGET_IP)/UDP(sport=12345, dport=CONTROL_PORT)/Raw(load=insecure_payload)
    send(pkt, verbose=False)
    print("[+] Insecure attempt sent. Daemon should dropped/ignore.")

if __name__ == "__main__":
    test_telemetry_flood()
    test_tls_downgrade_sim()
