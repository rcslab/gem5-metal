#pragma once

#include "arch/arm/insts/metal/common.hh"

namespace gem5
{
    namespace ArmISA
    {
        class Wpr64 : public MetalRegOp2
        {
        public:
            Wpr64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg);

            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
        };
    } // namespace ArmISA
} // namespace gem5