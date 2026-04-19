package com.aethernet.client.vpn

import android.content.Intent
import android.net.VpnService
import android.os.ParcelFileDescriptor
import android.util.Log
import com.google.protobuf.ByteString
import aether.proto.Packet.AetherPacket
import java.io.FileInputStream
import java.io.FileOutputStream
import java.net.DatagramPacket
import java.net.DatagramSocket
import java.net.InetAddress
import kotlin.concurrent.thread

class AetherVpnService : VpnService() {
    private var vpnInterface: ParcelFileDescriptor? = null
    private var isRunning = false
    private var proxySocket: DatagramSocket? = null
    
    // For local emulator connecting to host mock proxy
    private val PROXY_IP = "10.0.2.2"
    private val PROXY_PORT = 9000

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        setupVpn()
        return START_STICKY
    }

    private fun setupVpn() {
        val builder = Builder()
        builder.addAddress("10.0.0.2", 24)
        builder.addRoute("0.0.0.0", 0)
        builder.setSession("AetherNet")
        
        vpnInterface = builder.establish()
        
        startTrafficLoops()
    }

    private fun startTrafficLoops() {
        if (vpnInterface == null) return
        isRunning = true
        proxySocket = DatagramSocket()
        
        val tunFd = vpnInterface!!.fileDescriptor
        val tunIn = FileInputStream(tunFd)
        val tunOut = FileOutputStream(tunFd)

        // Thread 1: Outbound Engine (TUN -> UDP)
        thread {
            val buffer = ByteArray(65535)
            val proxyAddress = InetAddress.getByName(PROXY_IP)
            
            while (isRunning) {
                try {
                    val length = tunIn.read(buffer)
                    if (length > 0) {
                        // ML Context heuristic placeholder
                        val trafficClass = if (length > 1000) 1 else 2
                        
                        // Serialize with Protobuf
                        val packet = AetherPacket.newBuilder()
                            .setSourceId("android-client")
                            .setTargetId("auto-select-proxy-01")
                            .setSequenceNum(1L)
                            .setTimestampUs(System.currentTimeMillis() * 1000L)
                            .setTrafficClass(trafficClass)
                            .setPayload(ByteString.copyFrom(buffer, 0, length))
                            .build()
                            
                        val serialized = packet.toByteArray()
                        
                        val udpPacket = DatagramPacket(serialized, serialized.size, proxyAddress, PROXY_PORT)
                        proxySocket?.send(udpPacket)
                        Log.d("AetherVpn", "Outbound: Sent ${serialized.size} bytes via Protobuf")
                    }
                } catch (e: Exception) {
                    if (isRunning) Log.e("AetherVpn", "Outbound error", e)
                }
            }
        }

        // Thread 2: Inbound Engine (UDP -> TUN)
        thread {
            val buffer = ByteArray(65535)
            while (isRunning) {
                try {
                    val udpPacket = DatagramPacket(buffer, buffer.size)
                    proxySocket?.receive(udpPacket)
                    
                    // Deserialize Protobuf
                    val packet = AetherPacket.parseFrom(buffer.copyOfRange(0, udpPacket.length))
                    val rawPayload = packet.payload.toByteArray()
                    
                    // Inject to TUN
                    tunOut.write(rawPayload)
                    Log.d("AetherVpn", "Inbound: Injected ${rawPayload.size} bytes back to OS")
                } catch (e: Exception) {
                    if (isRunning) Log.e("AetherVpn", "Inbound error", e)
                }
            }
        }
    }

    override fun onDestroy() {
        isRunning = false
        proxySocket?.close()
        vpnInterface?.close()
        super.onDestroy()
    }
}
