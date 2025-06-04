#include "arch/arm/insts/metal/rmr.hh"

namespace gem5 {
    namespace ArmISA {
        Rmr64::Rmr64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg) : MetalRegOp2("rmr", _machInst, IntAluOp, _mreg, _greg)
        {
            setDestRegIdx(_numDestRegs++, intRegClass[gReg]);
            setSrcRegIdx(_numSrcRegs++, metalRegClass[mReg]);
            _numTypedDestRegs[intRegClass.type()]++;

            this->flags[IsInteger] = true;
        }

        Fault Rmr64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            RegVal v = xc->getRegOperand(this, 0);

            METAL_DBGPRINT(INSTS, RMR, "%s (0x%lx).\n", printMetalReg(mReg), v);

            xc->setRegOperand(this, 0, v);

            return NoFault;
        }
    }
}