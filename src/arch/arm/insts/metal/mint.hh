#pragma once

#include "arch/arm/insts/metal/common.hh"
#include "arch/arm/insts/metal/menter.hh"

namespace gem5 {
    namespace ArmISA {
    // menter
        class Mint64 : public Menter64
        {
        public:
            Mint64(ExtMachInst _machInst, uint8_t _imm);
            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
        };
    }
}