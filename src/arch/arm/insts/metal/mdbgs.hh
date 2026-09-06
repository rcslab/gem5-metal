#pragma once

#include "arch/arm/insts/metal/common.hh"

namespace gem5 {
    namespace ArmISA {
        namespace metal{ namespace inst{
            // mexit
            class Mdbgs64 : public MetalImmOp
            {
            public:
                static constexpr std::string_view FLAGS[] = {
                    "Metal",      // 0
                    "Exec",       // 1
                    "Faults",     // 2
                    "TLB",        // 3
                    "TLBVerbose", // 4
                    "Fetch",      // 5
                    "Commit",    // 6
                    "TLBOps"     // 7
                };
                // imm supports max 21 bits
                static_assert(std::size(FLAGS) <= 21);
                Mdbgs64(ExtMachInst _machInst, uint _imm);
                Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
            };
        }}
    }
}
