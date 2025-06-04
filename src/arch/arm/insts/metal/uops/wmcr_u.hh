#pragma once

#include "arch/arm/insts/metal/common.hh"
#include "cpu/metal_int_state.hh"
#include "arch/generic/memhelpers.hh"

namespace gem5 {
namespace ArmISA {
    class Wmcr64_u : public MetalMicroInst
    {
    private:
        RegIndex mReg;
        RegIndex gReg;
    public:
        Wmcr64_u(ExtMachInst _machInst, OpClass __opClass, RegIndex _mReg, RegIndex _gReg) :
            MetalMicroInst("wmcr_u", _machInst, __opClass), mReg(_mReg), gReg(_gReg)
        {
            setSrcRegIdx(_numSrcRegs++, intRegClass[gReg]);
            setDestRegIdx(_numDestRegs++, metalMiscRegClass[mReg]);
            _numTypedDestRegs[metalMiscRegClass.type()]++;

            this->flags[IsInteger] = true;
            this->flags[IsSerializeAfter] = true;
            this->flags[IsNonSpeculative] = true;
        }

        Fault preExec(ExecContext *xc, trace::InstRecord *traceData) const override
        {
            MetalInternalState mist;
            mist.set(xc->getPreExecMetalState());
            xc->setExecMetalState(mist);
            metal_reg::MSR_t msr = mist.getMSR();

            if (!msr.init && metal_reg::isInitReg(mReg) && metal_reg::isSetInit(mReg)) {
                msr.init = 1;
                mist.setMSR(msr);
            }

            xc->setPostExecMetalState(mist);

            return NoFault;
        }


        Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override 
        {
            const MetalInternalState & mist = xc->getExecMetalState();
            metal_reg::MSR_t msr = mist.getMSR();

            ThreadContext * tc = xc->tcBase();
            const RegVal mar = tc->readMetalMiscRegNoEffect(metal_reg::MAR);
            RegVal v = xc->getRegOperand(this, 0);

            bool allowWrite = false;
            if (!msr.init && metal_reg::isInitReg(mReg)) {
                // allow Metal initialization
                allowWrite = true;
            } else {
                allowWrite = metal_reg::getWritePerm(mar, mReg) && metal_reg::isInMetalMode(msr);
            }

            if (!allowWrite) {
                METAL_DBGPRINT(INSTS, WMCR_U, "Permission denied: writing %s (MAR = 0x%lx, MSR = 0x%lx).\n", printMetalMiscReg(mReg), mar, msr);
                return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }

            METAL_DBGPRINT(INSTS, WMCR_U, "%s => 0x%lx.\n", printMetalMiscReg(mReg), v);
            tc->setMetalMiscReg(mReg, v);

            return NoFault;
        }
    };

}
}