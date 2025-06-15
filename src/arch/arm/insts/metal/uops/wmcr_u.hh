#pragma once

#include "arch/arm/insts/metal/common.hh"
#include "arch/arm/regs/metal_misc.hh"
#include "base/types.hh"
#include "cpu/metal_int_state.hh"
#include "cpu/op_class.hh"
#include "cpu/thread_context.hh"

namespace gem5 {
namespace ArmISA {
    class Wmcr64_u : public MetalMicroInst
    {
    private:
        RegIndex mReg;
        RegIndex gReg;
        MetalInternalState saved_state;
    public:
        Wmcr64_u(ExtMachInst _machInst, RegIndex _mReg, RegIndex _gReg) :
            MetalMicroInst("wmcr_u", _machInst, IntAluOp), mReg(_mReg), gReg(_gReg)
        {
            setSrcRegIdx(_numSrcRegs++, intRegClass[gReg]);

            this->flags[IsInteger] = true;
            this->flags[IsSerializeAfter] = true;
            this->flags[IsNonSpeculative] = true;

            saved_state.reset();
        }

        Fault preExec(ExecContext *xc, trace::InstRecord *traceData) override
        {
            MetalInternalState mist = xc->getMetalState();
            saved_state.set(mist);
            metal_reg::MSR_t msr = mist.getMSR();
            const RegVal mar = xc->tcBase()->readMetalMiscRegNoEffect(metal_reg::MAR);

            if (!metal_reg::canAccessMiscReg(mReg, msr, mar, true)) {
                METAL_DBGPRINT(INSTS, WMCR_U, "Permission denied: %s. MSR: 0x%lx, MAR: 0x%lx.\n", printMetalMiscReg(mReg), msr, mar);
                return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }

            if (!msr.init && metal_reg::isInitReg(mReg) && metal_reg::isSetInit(mReg)) {
                msr.init = 1;
                mist.setMSR(msr);
            }

            xc->setMetalState(mist);

            return NoFault;
        }


        Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override
        {
            ThreadContext * tc = xc->tcBase();
            const RegVal mar = tc->readMetalMiscRegNoEffect(metal_reg::MAR);
            assert(metal_reg::canAccessMiscReg(mReg, saved_state.getMSR(), mar, true));

            RegVal v = xc->getRegOperand(this, 0);
            METAL_DBGPRINT(INSTS, WMCR_U, "%s => 0x%lx.\n", printMetalMiscReg(mReg), v);

            tc->setMetalMiscReg(mReg, v);
            if (traceData)
                traceData->setData(metalMiscRegClass, v);

            return NoFault;
        }
    };

}
}
