#include "arch/arm/insts/metal/wmr.hh"
#include "arch/arm/regs/metal_misc.hh"
#include "cpu/metal_int_state.hh"

namespace gem5 {
    namespace ArmISA {
        // wmr
        Wmr64::Wmr64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg) : MetalMRegRegOp("wmr", _machInst, IntAluOp, _mreg, _greg)
        {
            setSrcRegIdx(_numSrcRegs++, intRegClass[gReg]);
            setDestRegIdx(_numDestRegs++, metalRegClass[mReg]);
            _numTypedDestRegs[metalRegClass.type()]++;
            this->flags[IsInteger] = true;
        }

        Fault Wmr64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            const MetalInternalState state = xc->getMetalState();
            const metal_reg::MSR_t msr = state.getMSR();
            RegVal v = xc->getRegOperand(this, 0);

            METAL_DBGPRINT(INSTS, WMR, "%s @ Lv.%d => 0x%lx.\n", printMetalReg(mReg), msr.lv, v);

            xc->setRegOperand(this, 0, v);
            if (traceData)
                traceData->setData(metalRegClass, v);

            return NoFault;
        }

    }
}
