#pragma once

#include "arch/arm/insts/metal/common.hh"
#include "arch/arm/metal.hh"
#include "base/types.hh"
#include "cpu/metal_int_state.hh"
#include "cpu/op_class.hh"

namespace gem5 {
namespace ArmISA {
namespace metal { namespace inst {

    class Msinit64_u : public MetalMicroInst
    {
    public:
        Msinit64_u(ExtMachInst _machInst) :
            MetalMicroInst("msinit_u", _machInst, IntAluOp)
        {
        }

        Fault preExec(ExecContext *xc, trace::InstRecord *traceData) override
        {
            auto mist = xc->getMetalState();
            mist.setFlags(METAL_FLAG_INIT);
            xc->setMetalState(mist);

            return NoFault;
        }

        Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override
        {
            return NoFault;
        }
    };
}}
}
}
