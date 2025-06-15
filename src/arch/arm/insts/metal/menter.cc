#include "arch/arm/insts/metal/menter.hh"
#include "arch/arm/regs/metal.hh"
#include "arch/arm/regs/metal_misc.hh"
#include "arch/arm/utility.hh"
#include "enums/StaticInstFlags.hh"

namespace gem5 {
    namespace ArmISA {
        Menter64::Menter64(ExtMachInst _machInst, uint _imm) : MetalImmOp("menter", _machInst, IntAluOp, _imm)
        {
            this->flags[IsControl] = true;
            this->flags[IsIndirectControl] = true;
            this->flags[IsUncondControl] = true;
            this->flags[IsCall] = true;
            this->flags[IsPreExecOperandUpdate] = true;
        }

        void Menter64::doMenter(ExecContext *xc,
            const StaticInst * inst,
            Addr npc,
            Addr lpc,
            int mlr_idx)
        {
            PCState pcState;
            set(pcState, xc->tcBase()->pcState());
            // set new PC
            pcState.instNPC(npc);
            xc->pcState(pcState);

            // save link address
            xc->setRegOperand(inst, mlr_idx, lpc);

            // // write MSPSR
            // const CPSR spsr = ArmFault::dumpPState64(tc, inAArch64(tc), false);
            // tc->setMetalReg(RegId(metalRegClass, metal_reg::MSPSR), spsr);

            // // write MSFLAGS
            // const metal_reg::MFLAGS_t mflags = tc->readMetalMiscReg(metal_reg::MFLAGS);
            // tc->setMetalReg(RegId(metalRegClass, metal_reg::MSFLAGS), mflags);
        }

        // we know we will increase the metal level by 1 if successfully executed
        Fault Menter64::preExec(ExecContext *xc, trace::InstRecord *traceData)
        {
            MetalInternalState state = xc->getMetalState();
            metal_reg::MSR_t msr = state.getMSR();

            if (msr.lv >= metal_reg::MaxMetalLevel || imm >= ISA::MroutineTableMaxEntryNum) {
                return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }

            msr.lv = msr.lv + 1;
            state.setMSR(msr);
            xc->setMetalState(state);

            // manually flatten
            setDestRegIdx(_numDestRegs++, metalRegClass[metal_reg::MLR].flatten(xc));
            _numTypedDestRegs[metalRegClass.type()] += 1;

            return NoFault;
        }

        Fault Menter64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            ThreadContext * tc = xc->tcBase();
            metal_reg::MSR_t msr = xc->getMetalState().getMSR();
            ISA * isa = static_cast<ISA *>(tc->getIsaPtr());
            MRLB & mrlb = isa->getMrlbPtr();

            // lookup MRLB
            const MRLBEntry &mrlbEnt = mrlb.get(this->imm);

            assert(&mrlbEnt != &MRLB::NullEntry && msr.lv < metal_reg::MaxMetalLevel);

            METAL_DBGPRINT(INSTS, MENTER, "MRLB *hit* for mroutine %d. Addr = 0x%lx, valid = %d.\n", this->imm, mrlbEnt.getAddr(), mrlbEnt.isValid());

            if (!mrlbEnt.isValid()) {
                return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }

            const Addr lpc = xc->pcState().instAddr() + this->instSize();
            const Addr npc = purifyTaggedAddr(mrlbEnt.getAddr(), tc, currEL(tc), true);

            doMenter(xc, this, npc,  lpc, 0);

            METAL_DBGPRINT(INSTS, MENTER, "Entering Metal mode (Lv. %d): NextPC = 0x%lx, LinkPC (MLR %d) = 0x%lx.\n", msr.lv, npc, lpc);

            return NoFault;
        }
    }
}
