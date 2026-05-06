#include <iostream>
#include <vector>
#include "routing/ml_router.h"
#include "proto/packet.pb.h"
#include "port/protobuf_mutator.h"
#include "src/libfuzzer/libfuzzer_macro.h"

// Fuzzer that uses libprotobuf-mutator to feed structured data 
// into the MlRouter's inbound packet processing logic.

DEFINE_PROTO_FUZZER(const aether::proto::AetherPacket& packet) {
    static aether::MlRouter router;
    
    // Serialize the mutated proto into a string
    std::string serialized_data;
    if (packet.SerializeToString(&serialized_data)) {
        // Feed the serialized data into the target function
        // This stress-tests the ParseFromArray logic and subsequent DPI handling
        router.process_incoming(serialized_data.data(), serialized_data.size());
    }
}
