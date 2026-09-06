#include "arch/arm/insts/metal/menter.hh"
#include "arch/arm/regs/metal.hh"
#include "arch/arm/utility.hh"
#include "enums/StaticInstFlags.hh"

namespace gem5 { namespace ArmISA {
namespace metal { namespace inst {
        Menter64::Menter64( const char * mnem,ExtMachInst _machInst, uint _imm) : MetalImmOp(mnem, _machInst, IntAluOp, _imm)
        {
            this->flags[IsControl] = true;
            this->flags[IsIndirectControl] = true;
            this->flags[IsUncondControl] = true;
            this->flags[IsCall] = true;

            setDestRegIdx(_numDestRegs++, metalRegClass[metal::reg::MLR]);
            _numTypedDestRegs[metalRegClass.type()] += 1;
        }

        Menter64::Menter64(ExtMachInst _machInst, uint _imm) : Menter64("menter", _machInst, _imm)
        {
        }


        // we know we will increase the metal level by 1 if successfully executed
        Fault Menter64::preExec(ExecContext *xc, trace::InstRecord *traceData)
        {
            auto state = xc->getMetalState();

            if (state.getLevel() >= reg::MaxMetalLevel || imm >= MroutineTableMaxEntryNum) {
                return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }

            state.setLevel(state.getLevel() + 1);
            xc->setMetalState(state);

            return NoFault;
        }

        Fault Menter64::execute(ExecContext *xc, trace::InstRecord *traceData, Addr mlr) const
        {
            ThreadContext * tc = xc->tcBase();
            gem5::metal::InternalState state = xc->getMetalState();
            ISA * isa = static_cast<ISA *>(tc->getIsaPtr());
            MRLB & mrlb = isa->getMrlbPtr();

            // lookup MRLB
            const MRLBEntry &mrlbEnt = mrlb.get(this->imm);

            if (&mrlbEnt == &MRLB::NullEntry || !mrlbEnt.isValid()) {
                METAL_DBGPRINT(INSTS, MENTER, "MRLB *miss* or *invalid* for mroutine %d. \n", this->imm);
                return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }

            assert(state.getLevel() <= reg::MaxMetalLevel);

            METAL_DBGPRINT(INSTS, MENTER, "MRLB *hit* for mroutine %d. Addr = 0x%lx.\n", this->imm, mrlbEnt.getAddr());

            const Addr lpc = mlr;
            const Addr npc = purifyTaggedAddr(mrlbEnt.getAddr(), tc, currEL(tc), true);

            PCState pc;
            set(pc, xc->pcState());
            pc.instNPC(npc);
            xc->pcState(pc);

            // save link address
            xc->setRegOperand(this, 0, lpc);

            METAL_DBGPRINT(INSTS, MENTER, "Entering Metal mode (Lv. %d): NextPC = 0x%lx, LinkPC (MLR) = 0x%lx.\n", state.getLevel(), npc, lpc);

            return NoFault;
        }

        Fault Menter64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            return execute(xc, traceData, xc->pcState().instAddr() + this->instSize());
        }
}}
}}
