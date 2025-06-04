#include "arch/arm/insts/metal/rmcr.hh"

namespace gem5 {
    namespace ArmISA {
        Rmcr64::Rmcr64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg) : MetalRegOp2("rmcr", _machInst, IntAluOp, _mreg, _greg)
        {
            setDestRegIdx(_numDestRegs++, intRegClass[gReg]);
            setSrcRegIdx(_numSrcRegs++, metalMiscRegClass[mReg]);
            _numTypedDestRegs[intRegClass.type()]++;

            this->flags[IsInteger] = true;
            this->flags[IsSerializeAfter] = true;
            this->flags[IsNonSpeculative] = true;
        }

        Fault Rmcr64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            const MetalInternalState &mist = xc->getExecMetalState();
            const metal_reg::MSR_t msr = mist.getMSR();

            ThreadContext * tc = xc->tcBase();
            const RegVal mar = tc->readMetalMiscRegNoEffect(metal_reg::MAR);

            if (!metal_reg::getReadPerm(mar, mReg) || !metal_reg::isInMetalMode(msr)) {
                METAL_DBGPRINT(INSTS, RMCR, "Permission denied: reading %s (MAR = 0x%lx, MSR = 0x%lx).\n", printMetalMiscReg(mReg), mar, msr);
                return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }

            RegVal v = tc->readMetalMiscReg(mReg);

            METAL_DBGPRINT(INSTS, RMCR, "%s (0x%lx).\n", printMetalReg(mReg), v);

            xc->setRegOperand(this, 0, v);
            
            return NoFault;
        }

        std::string Rmcr64::generateDisassembly(
            Addr pc, const loader::SymbolTable *symtab) const
        {
            std::stringstream ss;
            ss << MetalDisasmPrefix;
            printMnemonic(ss, "", false);
            printMetalMiscReg(ss, mReg);
            ccprintf(ss, ", ");
            printIntReg(ss, gReg);
            return ss.str();
        }
    }
}