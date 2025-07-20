#include "arch/arm/insts/metal/rmgr.hh"
#include "arch/arm/regs/metal_misc.hh"

namespace gem5 {
    namespace ArmISA {
    namespace metal { namespace inst {
        Rmgr64::Rmgr64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg) : MetalMRegRegOp("rmgr", _machInst, IntAluOp, _mreg, _greg)
        {
            setSrcRegIdx(_numSrcRegs++, metalGlobalRegClass[mReg]);
            setDestRegIdx(_numDestRegs++, intRegClass[gReg]);
            _numTypedDestRegs[intRegClass.type()]++;

            this->flags[IsInteger] = true;
        }

        Fault Rmgr64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            const auto & mar = xc->tcBase()->readMetalMiscRegNoEffect(reg::MAR);
            if (!reg::canAccessGlobalReg(mReg, mar, false)) {
                METAL_DBGPRINT(INSTS, RMGR, "Permission denied: %s. MAR: 0x%lx.\n", printMetalMiscReg(mReg), mar);
                return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }

            const RegVal v = xc->getRegOperand(this, 0);

            METAL_DBGPRINT(INSTS, RMGR, "%s (0x%lx).\n", printMetalGlobalReg(mReg), v);

            xc->setRegOperand(this, 0, v);
            if (traceData)
                traceData->setData(metalRegClass, v);
            return NoFault;
        }

        std::string Rmgr64::generateDisassembly(
            Addr pc, const loader::SymbolTable *symtab) const
        {
            std::stringstream ss;
            ss << MetalDisasmPrefix;
            printMnemonic(ss, "", false);
            printMetalGlobalReg(ss, mReg);
            ccprintf(ss, ", ");
            printIntReg(ss, gReg);
            return ss.str();
        }
    }}
    }
}
