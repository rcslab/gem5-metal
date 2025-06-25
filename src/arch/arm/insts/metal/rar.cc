#include "arch/arm/insts/metal/rar.hh"
#include "arch/arm/regs/int.hh"
#include "arch/arm/regs/misc.hh"
#include "arch/arm/utility.hh"
#include "base/types.hh"
#include "cpu/metal_int_state.hh"
#include "cpu/reg_class.hh"

namespace gem5
{
    namespace ArmISA
    {
        namespace metal { namespace inst {
        Rar64::Rar64(ExtMachInst _machInst, RegIndex _reg, uint8_t _imm1, uint8_t _imm2) :
            MetalRegImm2Op("rar", _machInst, IntAluOp, _reg, _imm1, _imm2)
        {
            setDestRegIdx(_numDestRegs++, intRegClass[_reg]);
            // writing to metal class
            _numTypedDestRegs[intRegClass.type()]++;

            this->flags[IsInteger] = true;
            this->flags[IsPreExecOperandUpdate] = true;
        }

        Fault Rar64::preExec(ExecContext *xc, trace::InstRecord *traceData)
        {
            const RegIndex arid = imm1;
            const auto & state = xc->getMetalState();
            const int target_level = state.getLevel() - imm2;

            if (arid >= int_reg::NumArchRegs || target_level < 0) {
                METAL_DBGPRINT(INSTS, RAR, "Invalid arguments: dst = %d, src = %d, window = %d, MetalState = [%s].\n",
                    reg, imm1, imm2, state.toStr().c_str());
                return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }

            // manually flatten
            auto new_state = state;
            new_state.setLevel(target_level);
            setSrcRegIdx(_numSrcRegs++,
                IntRegClassOps::flattenWithStates(xc->tcBase()->readMiscRegNoEffect(MISCREG_CPSR),
                        new_state, intRegClass[arid]));

            return NoFault;
        }

        Fault Rar64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            METAL_DBGPRINT(INSTS, RAR, "dst = %s, src = %s, window = %d.\n",
                printIntReg(reg, 64).c_str(), printIntReg(imm1, 64).c_str(), imm2);

            RegVal val = xc->getRegOperand(this, 0);
            xc->setRegOperand(this, 0, val);

            if (traceData)
                traceData->setData(intRegClass, val);

            return NoFault;
        }
    }}
    } // namespace ArmISA
} // namespace gem5
