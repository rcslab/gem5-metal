#include "arch/arm/insts/metal/rpr.hh"

namespace gem5
{
    namespace ArmISA
    {
        Rpr64::Rpr64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg) : MetalRegOp2("rpr", _machInst, IntAluOp, _mreg, _greg)
        {
            setSrcRegIdx(_numSrcRegs++, metalRegClass[mReg]);
            setSrcRegIdx(_numSrcRegs++, metalRegClass[gReg]);
            // writing to metal class
            _numTypedDestRegs[metalRegClass.type()]++;

            this->flags[IsInteger] = true;
        }

        Fault Rpr64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            ThreadContext * tc = xc->tcBase();
            metal_reg::MSR_t msr = tc->readMetalMiscRegNoEffect(metal_reg::MSR);

            RegVal idx = tc->readMetalReg(this->mReg);
            ISA * isa = static_cast<ISA *>(xc->tcBase()->getIsaPtr());

            METAL_DBGPRINT(INSTS, RPR, "idxMReg = %s, srcGReg = %d, dstMReg = %s.\n", printMetalReg(this->mReg), idx, printMetalReg(this->gReg));

            if (idx >= int_reg::NumArchRegs || !metal_reg::isInMetalMode(msr)) {
                METAL_DBGPRINT(INSTS, RPR, "Permission denied: reading previous window reg %d (MSR = 0x%lx).\n", idx, msr);
                return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }
            

            RegVal v = isa->readIntRegAtLevel(idx, metal_reg::getMetalLevel(msr) - 1);
            tc->setMetalReg(gReg, v);

            return NoFault;
        }
    } // namespace ArmISA
} // namespace gem5
