#pragma once

#include "arch/arm/insts/metal/common.hh"

namespace gem5 {
    namespace ArmISA {
        // mexit
        class Mexit64 : public MetalImmOp
        {
        private:
            BitUnion8(MexitFlags)
            Bitfield<3> eim; // mask exception intercept for the next inst
            Bitfield<2> iim; // mask instruction intercept for the next inst
            Bitfield<1> rfi; // this is a return from intercept mroutine (restore cond flags (NZCV))
            Bitfield<0> id; // disable interrupt for the next inst
            EndBitUnion(MexitFlags)

        public:
            Mexit64(ExtMachInst _machInst, uint _imm);
            Fault preExec(ExecContext *xc, trace::InstRecord *traceData) override;
            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
        };
    }
}
