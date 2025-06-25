#pragma once

#include "arch/arm/insts/metal/menter.hh"

namespace gem5 {
    namespace ArmISA {
        namespace metal { namespace inst{
            class Mint64 : public Menter64
            {
            private:
                const RegVal mir0;
                const RegVal mir1;
                const RegVal mir2;
            public:
                Mint64(ExtMachInst _machInst, uint _imm, RegVal mir0, RegVal mir1, RegVal mir2);
                Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
                Fault preExec(ExecContext *xc, trace::InstRecord *traceData) override;
            };
        }}
    }
}
