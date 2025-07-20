#pragma once

#include "arch/arm/insts/metal/common.hh"
#include "arch/arm/metal.hh"
#include "arch/arm/regs/metal_misc.hh"
#include "base/types.hh"
#include "cpu/metal_int_state.hh"
#include "cpu/op_class.hh"
#include "cpu/thread_context.hh"

namespace gem5 {
namespace ArmISA {
namespace metal { namespace inst {

    class Wmcr64_u : public MetalMicroInst
    {
    private:
        RegIndex mReg;
        RegIndex gReg;
    public:
        Wmcr64_u(ExtMachInst _machInst, RegIndex _mReg, RegIndex _gReg) :
            MetalMicroInst("wmcr_u", _machInst, IntAluOp), mReg(_mReg), gReg(_gReg)
        {
            setSrcRegIdx(_numSrcRegs++, intRegClass[gReg]);

            this->flags[IsInteger] = true;
            this->flags[IsSerializeAfter] = true;
            this->flags[IsNonSpeculative] = true;
        }

        Fault preExec(ExecContext *xc, trace::InstRecord *traceData) override
        {
            const auto mist = xc->getMetalState();

            if (!reg::canAccessMiscReg(mReg, mist, true)) {
                METAL_DBGPRINT(INSTS, WMCR_U, "Permission denied: %s. MetalState: [%s].\n", printMetalMiscReg(mReg), mist.toStr().c_str());
                return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }

            // if (!(mist.getFlags() & METAL_FLAG_INIT) && reg::isSetInit(mReg)) {
            //     mist.setFlags(METAL_FLAG_INIT);
            // }

            //xc->setMetalState(mist);

            return NoFault;
        }


        Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override
        {
            ThreadContext * tc = xc->tcBase();

            RegVal v = xc->getRegOperand(this, 0);
            METAL_DBGPRINT(INSTS, WMCR_U, "%s => 0x%lx.\n", printMetalMiscReg(mReg), v);

            tc->setMetalMiscReg(mReg, v);
            if (traceData)
                traceData->setData(metalMiscRegClass, v);

            return NoFault;
        }
    };
}}
}
}
