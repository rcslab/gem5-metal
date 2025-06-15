#pragma once
#include "arch/arm/insts/metal/common.hh"

namespace gem5 {
    namespace ArmISA {
        class Rmr64 : public MetalMRegRegOp
        {
        public:
            Rmr64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg);

            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
        };
    }
}
