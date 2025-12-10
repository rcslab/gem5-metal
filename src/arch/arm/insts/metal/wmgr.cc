#include "arch/arm/insts/metal/wmgr.hh"
#include "cpu/reg_class.hh"
#include "enums/StaticInstFlags.hh"

namespace gem5 {
    namespace ArmISA {
    namespace metal { namespace inst {
        Wmgr64::Wmgr64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg) : MetalMRegRegOp("wmgr", _machInst, IntAluOp, _mreg, _greg)
        {
            setSrcRegIdx(_numSrcRegs++, intRegClass[gReg]);
            setDestRegIdx(_numDestRegs++, metalGlobalRegClass[mReg]);
            _numTypedDestRegs[metalGlobalRegClass.type()]++;

            this->flags[IsInteger] = true;
        }

        Fault Wmgr64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            const auto & mar = xc->tcBase()->readMetalMiscRegNoEffect(reg::MAR);
            if (!reg::canAccessGlobalReg(mReg, mar, true)) {
                METAL_DBGPRINT(INSTS, WMGR, "Permission denied: %s. MAR: 0x%lx.\n", printMetalGlobalReg(mReg), mar);
                return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }
            const RegVal v = xc->getRegOperand(this, 0);

            METAL_DBGPRINT(INSTS, WMGR, "%s => 0x%lx.\n", printMetalGlobalReg(mReg), v);

            xc->setRegOperand(this, 0, v);
            if (traceData)
                traceData->setData(metalGlobalRegClass, v);

            return NoFault;
        }

        std::string Wmgr64::generateDisassembly(
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
