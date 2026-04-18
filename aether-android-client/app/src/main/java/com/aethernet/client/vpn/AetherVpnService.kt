package com.aethernet.client.vpn

import android.content.Intent
import android.net.VpnService
import android.os.ParcelFileDescriptor

class AetherVpnService : VpnService() {
    private var vpnInterface: ParcelFileDescriptor? = null

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
        
        // Start processing TUN fd with thread/coroutine here
    }

    override fun onDestroy() {
        vpnInterface?.close()
        super.onDestroy()
    }
}
