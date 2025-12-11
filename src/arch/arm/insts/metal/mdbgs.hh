#pragma once

#include "arch/arm/insts/metal/common.hh"

namespace gem5 {
    namespace ArmISA {
        namespace metal{ namespace inst{
            // mexit
            class Mdbgs64 : public MetalNakedOp
            {
            public:
                Mdbgs64(ExtMachInst _machInst);
                Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
            };
        }}
    }
}
