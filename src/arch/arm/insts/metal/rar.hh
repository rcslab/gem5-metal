#pragma once

#include "arch/arm/insts/metal/common.hh"

namespace gem5
{
    namespace ArmISA
    {
        class Rar64 : public MetalRegImm2Op
        {
        public:
            Rar64(ExtMachInst _machInst, RegIndex _reg, uint8_t _imm1, uint8_t _imm2);
            Fault preExec(ExecContext *xc, trace::InstRecord *traceData) override;
            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
        };
    } // namespace ArmISA
} // namespace gem5
