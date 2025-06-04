#include "arch/arm/insts/metal/mexit.hh"

namespace gem5 {
    namespace ArmISA {
        Mexit64::Mexit64(ExtMachInst _machInst, uint8_t _imm) : MetalImmOp8("mexit", _machInst, IntAluOp, _imm)
        {
            setSrcRegIdx(_numSrcRegs++, metalRegClass[metal_reg::MLR]);

            this->flags[IsControl] = true;
            this->flags[IsIndirectControl] = true;
            this->flags[IsUncondControl] = true;
	        this->flags[IsInteger] = true;
            this->flags[IsCall] = true;

            const MexitFlags flags = static_cast<MexitFlags>(imm);
            if (flags.rfi) {
                //
                // "lightweight" exception: only restore COND flags
                //
                setSrcRegIdx(_numSrcRegs++, metalRegClass[metal_reg::MSPSR]);
                
                setDestRegIdx(_numDestRegs++, ccRegClass[cc_reg::Nz]);
                setDestRegIdx(_numDestRegs++, ccRegClass[cc_reg::C]);
                setDestRegIdx(_numDestRegs++, ccRegClass[cc_reg::V]);
                _numTypedDestRegs[ccRegClass.type()] += 3;

                this->flags[IsInteger] = true;
            }
        }

        Fault Mexit64::preExec(ExecContext *xc, trace::InstRecord *traceData) const
        {
            MetalInternalState post;
            post.set(xc->getPreExecMetalState());
            metal_reg::MSR_t msr = post.getMSR();

            if (!metal_reg::isInMetalMode(msr)) {
                return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }

            // execState should = preExec state as we need to read registers from that window
            xc->setExecMetalState(xc->getPreExecMetalState());

            const MexitFlags flags = static_cast<MexitFlags>(imm);

            if (flags.id) {
                msr.id = 1;
            }

            if (flags.eim) {
                msr.em = 1;
            }

            if (flags.iim) {
                msr.im = 1;
            }

            msr.lv = msr.lv - 1;
            post.setMSR(msr);
            // set post exec Metal state
            xc->setPostExecMetalState(post);

            return NoFault;

        }

        Fault Mexit64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            const MetalInternalState & mist = xc->getExecMetalState();
            metal_reg::MSR_t msr = mist.getMSR();

            if(!metal_reg::isInMetalMode(msr)) {
                return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }

            const RegVal ret = xc->getRegOperand(this, 0);
            const MexitFlags flags = static_cast<MexitFlags>(imm);

            // set new PC
            ThreadContext * tc = xc->tcBase();
            const Addr target_addr = purifyTaggedAddr(ret, tc, currEL(tc), true);
            PCState pcState;
            set(pcState, xc->pcState());
            pcState.instNPC(target_addr);
            xc->pcState(pcState);
            CPSR mspsr = 0;
            
            if (flags.rfi) {
                mspsr = xc->getRegOperand(this, 1);

                // restore NzCV
                xc->setRegOperand(this, 0, mspsr.nz);
                xc->setRegOperand(this, 1, mspsr.c);
                xc->setRegOperand(this, 2, mspsr.v);

                // const CPSR cpsr = xc->readMiscReg(MISCREG_CPSR);
                // const CPSR newCpsr = getPSTATEFromPSR(tc, cpsr, mspsr);
                // // restore flags that are in separate regs
                // xc->setMiscReg(MISCREG_NZCV, newCpsr);
                // // restore other flags that are stored in CPSR
                // xc->setMiscReg(MISCREG_CPSR, newCpsr);

                // restore MFLAGS
                // metal_reg::MFLAGS_t mflags = xc->getRegOperand(this, 2);
                
                // msflags = tc->readMetalReg(metal_reg::MSFLAGS);
                // tc->setMetalMiscReg(metal_reg::MFLAGS, msflags);
            }

            METAL_DBGPRINT(INSTS, MEXIT, "Exiting Metal mode: MLR = 0x%lx, flags = [id = %d, rfi = %d (MSPSR = 0x%lx), iim = %d, eim = %d]\n",
                                                ret, flags.id, flags.rfi, mspsr, flags.iim, flags.eim);

            //
            // decrease Metal level again
            //
            // msr.lv = msr.lv - 1;
            // mist.setMSR(msr);

            return NoFault;
        }
    }
}