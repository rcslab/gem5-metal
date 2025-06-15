#include "arch/arm/insts/metal/rmr.hh"

namespace gem5 {
    namespace ArmISA {
        Rmr64::Rmr64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg) : MetalMRegRegOp("rmr", _machInst, IntAluOp, _mreg, _greg)
        {
            setDestRegIdx(_numDestRegs++, intRegClass[gReg]);
            setSrcRegIdx(_numSrcRegs++, metalRegClass[mReg]);
            _numTypedDestRegs[intRegClass.type()]++;

            this->flags[IsInteger] = true;
        }

        Fault Rmr64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            const MetalInternalState state = xc->getMetalState();
            const metal_reg::MSR_t msr = state.getMSR();

            RegVal v = xc->getRegOperand(this, 0);

            METAL_DBGPRINT(INSTS, RMR, "%s @ Lv.%d (0x%lx).\n", printMetalReg(mReg), msr.lv, v);

            xc->setRegOperand(this, 0, v);
            if (traceData)
                traceData->setData(metalRegClass, v);
            return NoFault;
        }
    }
}
