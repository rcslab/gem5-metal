#include "arch/arm/insts/metal/mexit.hh"

namespace gem5 {
    namespace ArmISA {
        Mexit64::Mexit64(ExtMachInst _machInst, RegIndex _mreg) : MetalRegOp("mexit", _machInst, IntAluOp, _mreg)
        {
            this->flags[IsControl] = true;
            this->flags[IsIndirectControl] = true;
            this->flags[IsInteger] = true;
            this->flags[IsReturn] = true;
            this->flags[IsUncondControl] = true;
        }

        Fault Mexit64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            ThreadContext * tc = xc->tcBase();
            // get mroutine table's base address
            const RegVal ret = tc->readMetalReg(metal_reg::MLR);
            metal_reg::MSR_t msr = tc->readMetalMiscReg(metal_reg::MSR);
            const RegVal reg = tc->readMetalReg(mReg);
            const MexitFlags flags = static_cast<MexitFlags>(reg);

            // must be in Metal mode to mexit
            if (!metal_reg::isInMetalMode(msr)) {
                return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }

            // set new PC
            const Addr target_addr = purifyTaggedAddr(ret, tc, currEL(tc), true);
            PCState pcState;
            set(pcState, xc->pcState());
            pcState.instNPC(target_addr);
            xc->pcState(pcState);

            CPSR newCpsr = 0;
            CPSR mspsr = 0;
            if (flags.rfi) {
                // restore PSTATE from MSPSR
                mspsr = tc->readMetalReg(metal_reg::MSPSR);
                const CPSR cpsr = xc->readMiscReg(MISCREG_CPSR);
                newCpsr = getPSTATEFromPSR(tc, cpsr, mspsr);
                // restore flags that are in separate regs
                xc->setMiscReg(MISCREG_NZCV, newCpsr);
                // restore other flags that are stored in CPSR
                xc->setMiscReg(MISCREG_CPSR, newCpsr);
            }

            METAL_DBGPRINT(INSTS, MEXIT, "Exiting Metal mode: MLR = 0x%lx, flags = 0x%lx [id = %d, rfi = %d (MSPSR = 0x%lx, NCPSR = 0x%lx), iim = %d, eim = %d]\n",
                                                ret, reg, flags.id, flags.rfi, mspsr, newCpsr, flags.iim, flags.eim);

            if (flags.iim) {
                msr.im = 1;
            }
            if (flags.eim) {
                msr.em = 1;
            }
            if (flags.id) {
                msr.id = 1;
            }

            // decrease Metal level
            msr.lv = msr.lv - 1;
            tc->setMetalMiscReg(metal_reg::MSR, msr);

            return NoFault;
        }
    }
}