#include <iostream>
#include "routing/ml_router.h"
#include "proto/control.pb.h"
#include "port/protobuf_mutator.h"
#include "src/libfuzzer/libfuzzer_macro.h"

// Fuzzer targeting the UDP Control Plane telemetry ingestion.
// This monitors for potential overflows or logic errors in health calculations.

DEFINE_PROTO_FUZZER(const aether::proto::NodeTelemetry& telemetry) {
    static aether::MlRouter router;
    
    std::string serialized_data;
    if (telemetry.SerializeToString(&serialized_data)) {
        router.process_telemetry(serialized_data.data(), serialized_data.size());
    }
}
