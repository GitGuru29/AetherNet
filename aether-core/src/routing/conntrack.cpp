#include "conntrack.h"
#include <iostream>

namespace aether {

Conntrack::Conntrack() : next_session_id(1) {
    std::cout << "[Conntrack] Initialized Thread-Safe NAT Session Manager." << std::endl;
}

int Conntrack::get_or_create_session(const FlowTuple& tuple) {
    // Phase 1: Fast Read-Only Lock
    {
        std::shared_lock<std::shared_mutex> lock(map_mutex);
        auto it = session_table.find(tuple);
        if (it != session_table.end()) {
            it->second.last_seen = std::chrono::steady_clock::now();
            return it->second.session_id;
        }
    }

    // Phase 2: Exclusive Write Lock (to insert new session)
    std::unique_lock<std::shared_mutex> lock(map_mutex);
    // Double-check pattern to prevent race conditions during lock escalation
    auto it = session_table.find(tuple);
    if (it != session_table.end()) {
        it->second.last_seen = std::chrono::steady_clock::now();
        return it->second.session_id;
    }

    SessionEntry new_entry;
    new_entry.session_id = next_session_id++;
    new_entry.last_seen = std::chrono::steady_clock::now();
    
    session_table[tuple] = new_entry;
    
    std::cout << "[Conntrack] Created new NAT session ID " << new_entry.session_id 
              << " for Protocol " << (int)tuple.protocol << std::endl;
              
    return new_entry.session_id;
}

void Conntrack::expire_stale_sessions() {
    std::unique_lock<std::shared_mutex> lock(map_mutex);
    auto now = std::chrono::steady_clock::now();
    
    for (auto it = session_table.begin(); it != session_table.end(); ) {
        // Expire sessions inactive for more than 120 seconds
        if (std::chrono::duration_cast<std::chrono::seconds>(now - it->second.last_seen).count() > 120) {
            std::cout << "[Conntrack] Expiring stale NAT session ID " << it->second.session_id << std::endl;
            it = session_table.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace aether
