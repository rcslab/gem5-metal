#include "arch/arm/insts/metal/wpr.hh"

namespace gem5
{
    namespace ArmISA
    {
        Wpr64::Wpr64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg) : MetalRegOp2("wpr", _machInst, IntAluOp, _mreg, _greg)
        {
            setSrcRegIdx(_numSrcRegs++, metalRegClass[mReg]);
            setSrcRegIdx(_numDestRegs++, metalRegClass[gReg]);
            // writing to int class
            _numTypedDestRegs[intRegClass.type()]++;

            this->flags[IsInteger] = true;
            this->flags[IsSerializeAfter] = true;
            this->flags[IsNonSpeculative] = true;
        }

        Fault Wpr64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            const MetalInternalState & mist = xc->getExecMetalState();
            const metal_reg::MSR_t msr = mist.getMSR();

            const RegVal idx = xc->getRegOperand(this, 0);
            const RegVal v = xc->getRegOperand(this, 1);

            METAL_DBGPRINT(INSTS, WPR, "idxMReg = %s, dstGReg = %d, srcMReg = %s.\n", printMetalReg(this->mReg), idx, printMetalReg(this->gReg));

            if (idx >= int_reg::NumArchRegs || !metal_reg::isInMetalMode(msr)) {
                METAL_DBGPRINT(INSTS, RPR, "Permission denied: reading previous window reg %d (MSR = 0x%lx).\n", idx, msr);
                return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }

            ISA * isa = static_cast<ISA *>(xc->tcBase()->getIsaPtr());
            isa->setIntRegAtLevel(idx, v, metal_reg::getMetalLevel(msr) - 1);

            return NoFault;
        }

    } // namespace ArmISA
} // namespace gem5
