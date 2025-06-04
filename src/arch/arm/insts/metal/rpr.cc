#include "arch/arm/insts/metal/rpr.hh"

namespace gem5
{
    namespace ArmISA
    {
        Rpr64::Rpr64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg) : MetalRegOp2("rpr", _machInst, IntAluOp, _mreg, _greg)
        {
            setSrcRegIdx(_numSrcRegs++, metalRegClass[mReg]);
            setDestRegIdx(_numSrcRegs++, metalRegClass[gReg]);
            // writing to metal class
            _numTypedDestRegs[metalRegClass.type()]++;

            //
            // read previous register window's register because it's hard to track
            //
            this->flags[IsInteger] = true;
            this->flags[IsSerializeAfter] = true;
            this->flags[IsNonSpeculative] = true;
        }

        Fault Rpr64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            const MetalInternalState &mist = xc->getExecMetalState();
            const metal_reg::MSR_t msr = mist.getMSR();

            RegVal idx = xc->getRegOperand(this, 0);
            METAL_DBGPRINT(INSTS, RPR, "idxMReg = %s, srcGReg = %d, dstMReg = %s.\n", printMetalReg(this->mReg), idx, printMetalReg(this->gReg));

            if (idx >= int_reg::NumArchRegs || !metal_reg::isInMetalMode(msr)) {
                METAL_DBGPRINT(INSTS, RPR, "Permission denied: reading previous window reg %d (MSR = 0x%lx).\n", idx, msr);
                return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }

            ISA * isa = static_cast<ISA *>(xc->tcBase()->getIsaPtr());
            RegVal v = isa->readIntRegAtLevel(idx, metal_reg::getMetalLevel(msr) - 1);
            xc->setRegOperand(this, 0, v);

            return NoFault;
        }
    } // namespace ArmISA
} // namespace gem5
