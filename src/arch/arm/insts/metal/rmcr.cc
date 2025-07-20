#include "arch/arm/insts/metal/rmcr.hh"
#include "arch/arm/regs/metal_misc.hh"

namespace gem5 {
    namespace ArmISA {
        namespace metal { namespace inst {
        Rmcr64::Rmcr64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg) : MetalMRegRegOp("rmcr", _machInst, IntAluOp, _mreg, _greg)
        {
            setDestRegIdx(_numDestRegs++, intRegClass[gReg]);
            _numTypedDestRegs[intRegClass.type()]++;

            this->flags[IsInteger] = true;
        }

        Fault Rmcr64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            const auto &mist = xc->getMetalState();

            ThreadContext * tc = xc->tcBase();

            if (!reg::canAccessMiscReg(mReg, mist, false)) {
                METAL_DBGPRINT(INSTS, RMCR, "Permission denied: %s. MetalState: [%s].\n", printMetalMiscReg(mReg), mist.toStr().c_str());
                return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }

            RegVal v = tc->readMetalMiscReg(mReg);

            METAL_DBGPRINT(INSTS, RMCR, "%s (0x%lx).\n", printMetalReg(mReg), v);

            xc->setRegOperand(this, 0, v);
            if (traceData)
                traceData->setData(metalMiscRegClass, v);
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
    }}
    }
}
