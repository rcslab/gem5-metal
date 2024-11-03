#include "arch/arm/insts/metal/wmr.hh"

namespace gem5 {
    namespace ArmISA {
        // wmr
        Wmr64::Wmr64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg) : MetalRegOp2("wmr", _machInst, IntAluOp, _mreg, _greg)
        {
            setSrcRegIdx(_numSrcRegs++, gem5::ArmISA::couldBeZero(gReg) ? RegId() : intRegClass[gReg]);
            setDestRegIdx(_numDestRegs++, metalRegClass[mReg]);
            _numTypedDestRegs[metalRegClass.type()]++;
            this->flags[IsInteger] = true;
        }

        Fault Wmr64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            ThreadContext * tc = xc->tcBase();

            RegVal v = xc->getRegOperand(this, 0);

            METAL_DBGPRINT(INSTS, WMR, "%s => 0x%lx.\n", printMetalReg(mReg), v);

            tc->setMetalReg(mReg, v);

            return NoFault;
        }

    }
}