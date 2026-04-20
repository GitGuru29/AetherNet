import socket
import threading
import time
import control_pb2

UDP_IP = "127.0.0.1"
UDP_PORT = 9000

# Spin up a simple UDP server
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

# Control Plane Background Broadcaster
def telemetry_broadcaster():
    control_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    while True:
        time.sleep(5)
        telemetry = control_pb2.NodeTelemetry()
        telemetry.node_id = "mock-proxy-01"
        telemetry.health_score = 0.88
        telemetry.latency_ms = 18.5
        telemetry.cpu_load_percent = 42
        
        control_sock.sendto(telemetry.SerializeToString(), ("127.0.0.1", 9002))
        print(f"📡 Broadcasted NodeTelemetry heartbeat to Core Control Plane (Port 9002)")

threading.Thread(target=telemetry_broadcaster, daemon=True).start()

print(f"🔥 Torch Mock Server listening for AetherPackets on port {UDP_PORT}...")

while True:
    data, addr = sock.recvfrom(65535) # buffer size is 65535 bytes
    # print(f"[{addr[0]}:{addr[1]}] -> Received {len(data)} bytes from AetherNet core!")
    
    # ECHO the identical payload back to the daemon to simulate proxy response
    print(f"   ↳ Bouncing packet back to client to test dual-threaded pipeline...")
    sock.sendto(data, addr)
