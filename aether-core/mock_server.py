import socket

UDP_IP = "127.0.0.1"
UDP_PORT = 9000

# Spin up a simple UDP server
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

print(f"🔥 Torch Mock Server listening for AetherPackets on port {UDP_PORT}...")

while True:
    data, addr = sock.recvfrom(65535) # buffer size is 65535 bytes
    print(f"[{addr[0]}:{addr[1]}] -> Received {len(data)} bytes from AetherNet core!")
    
    # ECHO the identical payload back to the daemon to simulate proxy response
    print(f"   ↳ Bouncing packet back to client to test dual-threaded pipeline...")
    sock.sendto(data, addr)
