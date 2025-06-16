#include "arch/arm/insts/metal/war.hh"

namespace gem5
{
    namespace ArmISA
    {
        War64::War64(ExtMachInst _machInst, RegIndex _reg, uint8_t _imm1, uint8_t _imm2) :
            MetalRegImm2Op("war", _machInst, IntAluOp, _reg, _imm1, _imm2)
        {
            setSrcRegIdx(_numSrcRegs++, intRegClass[_reg]);
            this->flags[IsInteger] = true;
            this->flags[IsPreExecOperandUpdate] = true;
        }

        Fault War64::preExec(ExecContext *xc, trace::InstRecord *traceData)
        {
            const RegIndex arid = imm1;
            const metal_reg::MSR_t msr = xc->getMetalState().getMSR();
            const int target_level = metal_reg::getMetalLevel(msr) - imm2;

            if (arid >= int_reg::NumArchRegs || target_level < 0) {
                METAL_DBGPRINT(INSTS, RAR, "Invalid arguments: dst = %d, src = %d, window = %d, MSR = 0x%lx.\n",
                    reg, imm1, imm2, msr);
                return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }

            // manually flatten
            metal_reg::MSR_t new_msr = msr;
            new_msr.lv = target_level;
            setDestRegIdx(_numDestRegs++,
                IntRegClassOps::flattenWithStates(xc->tcBase()->readMiscRegNoEffect(MISCREG_CPSR),
                        new_msr, intRegClass[arid]));
            _numTypedDestRegs[intRegClass.type()]++;

            return NoFault;
        }

        Fault War64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            METAL_DBGPRINT(INSTS, WAR, "src = %s, dst = %s, window = %d.\n",
                printIntReg(reg, 64).c_str(), printIntReg(imm1, 64).c_str(), imm2);

            RegVal val = xc->getRegOperand(this, 0);
            xc->setRegOperand(this, 0, val);
            if (traceData)
                traceData->setData(intRegClass, val);
            return NoFault;
        }
    } // namespace ArmISA
} // namespace gem5
