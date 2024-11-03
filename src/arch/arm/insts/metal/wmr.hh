#pragma once

#include "arch/arm/insts/metal/common.hh"

namespace gem5 {
    namespace ArmISA {
        class Wmr64 : public MetalRegOp2
        {
        public:
            Wmr64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg);

            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
        };
    }
}