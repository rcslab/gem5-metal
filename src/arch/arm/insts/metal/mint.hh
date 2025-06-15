#pragma once

#include "arch/arm/insts/metal/menter.hh"

namespace gem5 {
    namespace ArmISA {
    // menter
        class Mint64 : public Menter64
        {
        private:
            RegVal saved_mflags;
        public:
            Mint64(ExtMachInst _machInst, uint _imm);
            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
            Fault preExec(ExecContext *xc, trace::InstRecord *traceData) override;
        };
    }
}
