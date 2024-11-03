#pragma once

#include "arch/arm/insts/metal/common.hh"

namespace gem5 {
    namespace ArmISA {
        class Rtlb64 : public MetalRegOp3
        {
        public:
            Rtlb64(ExtMachInst _machInst, RegIndex _rl, RegIndex _rm,
                   RegIndex _rn);

            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
        };
    }
}