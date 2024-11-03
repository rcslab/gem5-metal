#include "arch/arm/insts/metal/rmr.hh"

namespace gem5 {
    namespace ArmISA {
        Rmr64::Rmr64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg) : MetalRegOp2("rmr", _machInst, IntAluOp, _mreg, _greg)
        {
            setDestRegIdx(_numDestRegs++, gem5::ArmISA::couldBeZero(gReg) ? RegId() : intRegClass[gReg]);
            setSrcRegIdx(_numSrcRegs++, metalRegClass[mReg]);
            _numTypedDestRegs[intRegClass.type()]++;

            this->flags[IsInteger] = true;
        }

        Fault Rmr64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            ThreadContext * tc = xc->tcBase();

            RegVal v = tc->readMetalReg(mReg);

            METAL_DBGPRINT(INSTS, RMR, "%s (0x%lx).\n", printMetalReg(mReg), v);

            xc->setRegOperand(this, 0, v);

            return NoFault;
        }
    }
}