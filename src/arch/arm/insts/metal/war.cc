#include "arch/arm/insts/metal/war.hh"

namespace gem5 {
    namespace ArmISA {
        War64::War64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg) : MetalRegOp2("war", _machInst, IntAluOp, _mreg, _greg)
        {
            setSrcRegIdx(_numSrcRegs++, metalRegClass[mReg]);
            setSrcRegIdx(_numDestRegs++, metalRegClass[gReg]);
            // writing to int class
            _numTypedDestRegs[intRegClass.type()]++;

            this->flags[IsInteger] = true;
        }

        Fault War64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            ThreadContext * tc = xc->tcBase();
            metal_reg::MSR_t msr = tc->readMetalMiscRegNoEffect(metal_reg::MSR);
            RegVal idx = tc->readMetalReg(this->mReg);

            METAL_DBGPRINT(INSTS, WAR, "idxMReg = %s, dstGReg = %d, srcMReg = %s.\n", printMetalReg(this->mReg), idx, printMetalReg(this->gReg));

            if (idx >= int_reg::NumArchRegs) {
                METAL_DBGPRINT(INSTS, WAR, "Attempting to write out of bound arch reg index: %d.\n", idx);
                return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }

            RegVal v = tc->readMetalReg(this->gReg);
            xc->setReg(intRegClass[idx], v);

            return NoFault;
        }

    }
}
