#pragma once

#include "arch/arm/insts/metal/common.hh"

namespace gem5 {
    namespace ArmISA {
        // mexit
        class Mexit64 : public MetalRegOp
        {
        private:
            BitUnion8(MexitFlags)
            Bitfield<3> eim; // mask exception intercept for the next inst
            Bitfield<2> iim; // mask instruction intercept for the next inst
            Bitfield<1> rfi; // this is a return from intercept (Inst & Exc) mroutine (restore CPSR from MSPSR)
            Bitfield<0> id; // disable interrupt for the next inst
            EndBitUnion(MexitFlags)

        public:
            Mexit64(ExtMachInst _machInst, RegIndex _mreg);

            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
        };
    }
}