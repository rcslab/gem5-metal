#include "arch/arm/insts/metal/rar.hh"

namespace gem5 {
    namespace ArmISA {
        Rar64::Rar64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg) : MetalRegOp2("rar", _machInst, IntAluOp, _mreg, _greg)
        {
            setSrcRegIdx(_numSrcRegs++, metalRegClass[mReg]);
            setDestRegIdx(_numDestRegs++, metalRegClass[gReg]);
            _numTypedDestRegs[metalRegClass.type()]++;

            this->flags[IsInteger] = true;
        }

        Fault Rar64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            ThreadContext *tc = xc->tcBase();
            metal_reg::MSR_t msr = tc->readMetalMiscReg(metal_reg::MSR);
            RegVal idx = tc->readMetalReg(this->mReg);

            METAL_DBGPRINT(INSTS, RAR, "idxMReg = %s, srcGReg = %d, dstMReg = %s.\n", printMetalReg(this->mReg), idx, printMetalReg(this->gReg));

            if (idx >= int_reg::NumArchRegs) {
                METAL_DBGPRINT(INSTS, RAR, "Attempting to read out of bound arch reg index: %d.\n", idx);
                return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }

            RegVal v = xc->getReg(intRegClass[idx]);
            tc->setMetalReg(this->gReg, v);

            return NoFault;
        }
    }
}
