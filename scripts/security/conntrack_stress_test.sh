#!/bin/bash
# AetherNet Conntrack State Exhaustion Resilience Test

echo "[*] Initializing Conntrack Stress Test..."

# 1. Lower the conntrack limit in the current namespace (requires sudo/capability)
echo 1000 | sudo tee /proc/sys/net/netfilter/nf_conntrack_max

# 2. Check current count
CURRENT_COUNT=$(cat /proc/sys/net/netfilter/nf_conntrack_count)
echo "[*] Current Conntrack Count: $CURRENT_COUNT (Max: 1000)"

# 3. Use hping3 to simulate a SYN flood to fill the table
echo "[*] Flooding table with SYN packets..."
sudo hping3 -S -p 80 --fast --rand-source 127.0.0.1 -c 2000 > /dev/null 2>&1

# 4. Verify table is full
FINAL_COUNT=$(cat /proc/sys/net/netfilter/nf_conntrack_count)
echo "[*] Final Conntrack Count: $FINAL_COUNT"

if [ "$FINAL_COUNT" -ge 990 ]; then
    echo "[+] SUCCESS: Conntrack table successfully saturated."
else
    echo "[-] FAILURE: Table was not saturated. Check network namespace settings."
fi

# 5. TEST: Can we still reach the daemon?
# Attempt a simple UDP health check to the control plane
echo "[*] Verifying AetherNet Control Plane responsiveness during flood..."
nc -u -z -v 127.0.0.1 9002 2>&1 | grep "succeeded"

# 6. Cleanup
sudo conntrack -F
echo 65536 | sudo tee /proc/sys/net/netfilter/nf_conntrack_max
echo "[*] Conntrack table flushed and limit restored."
