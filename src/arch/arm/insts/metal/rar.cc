#include "arch/arm/insts/metal/rar.hh"

namespace gem5 {
    namespace ArmISA {
        Rar64::Rar64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg) : MetalRegOp2("rar", _machInst, IntAluOp, _mreg, _greg)
        {
            setSrcRegIdx(_numSrcRegs++, metalRegClass[mReg]);
            setDestRegIdx(_numDestRegs++, metalRegClass[gReg]);
            _numTypedDestRegs[metalRegClass.type()]++;

            this->flags[IsInteger] = true;
            // XXX: serialize here because it's messy to resolve runtime dependency
            // when the dependency itself is held in a renamed register
            // i.e. need to go back to rename stage after execute stage
            this->flags[IsSerializeAfter] = true;
            this->flags[IsNonSpeculative] = true;
        }

        Fault Rar64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            RegVal idx = xc->getRegOperand(this, 0);

            METAL_DBGPRINT(INSTS, RAR, "idxMReg = %s(%d), dstMReg = %s.\n", printMetalReg(this->mReg), idx, printMetalReg(this->gReg));

            if (idx >= int_reg::NumArchRegs) {
                METAL_DBGPRINT(INSTS, RAR, "Attempting to read out of bound arch reg index: %d.\n", idx);
                return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }
            
            // XXX: this part requires serialization
            ThreadContext * tc = xc->tcBase();
            RegVal v = xc->tcBase()->getReg(intRegClass[idx]);

            xc->setRegOperand(this, 0, v);

            return NoFault;
        }
    }
}
