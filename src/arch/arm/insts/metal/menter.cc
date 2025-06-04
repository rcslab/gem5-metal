#include "arch/arm/insts/metal/menter.hh"
#include "arch/generic/memhelpers.hh"

namespace gem5 {
    namespace ArmISA {
        Menter64::Menter64(ExtMachInst _machInst, uint8_t _imm) : MetalImmOp8("menter", _machInst, IntAluOp, _imm)
        {
            this->flags[IsControl] = true;
            this->flags[IsIndirectControl] = true;
            this->flags[IsUncondControl] = true;
	        this->flags[IsInteger] = true;
            this->flags[IsCall] = true;

            setDestRegIdx(_numDestRegs++, metalRegClass[metal_reg::MLR]);
            _numTypedDestRegs[metalRegClass.type()] += 1;
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

            METAL_DBGPRINT(INSTS, MENTER, "Entering Metal mode (Lv. %d -> %d): NextPC = 0x%lx, LinkPC (MLR) = 0x%lx.\n", 
                static_cast<metal_reg::MSR_t>(xc->getPreExecMetalState().getMSR()).lv,
                static_cast<metal_reg::MSR_t>(xc->getPostExecMetalState().getMSR()).lv,
                npc, lpc);
        }

        // void Menter64::doMenter(ThreadContext * tc, Addr npc, const ArmStaticInst &inst)
        // {
        //     doMenter(tc, npc, tc->pcState().instAddr() + inst.instSize());
        // }

        // void Menter64::calcLoadAddr(Addr base, unsigned long align, unsigned int idx, Addr & _loadAddr, unsigned int & _count)
        // {
        //     // ensure base is ISA::MroutineTableEntry aligned (enforced by WMR)
        //     assert((base & (sizeof(ISA::MroutineTableEntry) - 1)) == 0);
        //     // ensure align is power of 2
        //     assert(isPowerOf2(align));
        //     // ensure we can load something
        //     assert(sizeof(ISA::MroutineTableEntry) <= align);

        //     const Addr mroutineAddr = base + sizeof(ISA::MroutineTableEntry) * idx;
        //     const Addr loadAddr = mroutineAddr & (~(align - 1));
        //     assert(mroutineAddr >= loadAddr && mroutineAddr - loadAddr <= align);

        //     Addr nextAlign = loadAddr + align;
        //     // check that the entry is not crossing align boundry
        //     assert(mroutineAddr + sizeof(ISA::MroutineTableEntry) <= nextAlign);

        //     // bound check for the entire table
        //     const Addr limitAddr = base + sizeof(ISA::MroutineTableEntry) * ISA::MroutineTableMaxEntryNum;
        //     if (nextAlign > limitAddr) {
        //         nextAlign = limitAddr;
        //     }

        //     assert((nextAlign - loadAddr) % sizeof(ISA::MroutineTableEntry) == 0);
        //     _loadAddr = loadAddr;
        //     _count = (nextAlign - loadAddr) / sizeof(ISA::MroutineTableEntry);
        // }

        // Fault Menter64::initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const
        // {
        //     ThreadContext * tc = xc->tcBase();
        //     ISA * isa = static_cast<ISA *>(tc->getIsaPtr());
        //     MRLB & mrlb = isa->getMrlbPtr();

        //     // lookup MRLB
        //     const MRLBEntry &mrlbEnt = mrlb.get(this->imm);

        //     Fault fault = NoFault;
        //     if (&mrlbEnt == &MRLB::NullEntry) {
        //         const RegVal mbr = tc->readMetalMiscReg(metal_reg::MBR);

        //         // ensure mbr is 8 byte aligned (enforced by WMR)
        //         assert((mbr & (sizeof(ISA::MroutineTableEntry) - 1)) == 0);

        //         Addr loadAddr;
        //         unsigned int count;

        //         this->calcLoadAddr(mbr, tc->getSystemPtr()->cacheLineSize(), this->imm, loadAddr, count);

        //         METAL_DBGPRINT(INSTS, MENTER, "MRLB *miss* for mrouine %d. Mem load addr = 0x%lx, count = %d.\n", this->imm, loadAddr, count);

        //         fault = initiateMemRead(xc, loadAddr, tc->getSystemPtr()->cacheLineSize(), ArmISA::MMU::AllowUnaligned);
        //     } else {
        //         METAL_DBGPRINT(INSTS, MENTER, "MRLB *hit* for mroutine %d. Addr = 0x%lx, valid = %d.\n", this->imm, mrlbEnt.getAddr(), mrlbEnt.isValid());
        //         if (!mrlbEnt.isValid()) {
        //             return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
        //         } else {
        //             doMenter(tc, purifyTaggedAddr(mrlbEnt.getAddr(), tc, currEL(tc), true), *this);
        //         }
        //     }

        //     return fault;
        // }

        // Fault Menter64::completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const
        // {
        //     ThreadContext * tc = xc->tcBase();
        //     ISA * isa = static_cast<ISA *>(tc->getIsaPtr());
        //     MRLB & mrlb = isa->getMrlbPtr();
        //     const size_t cacheLineSz = tc->getSystemPtr()->cacheLineSize();

        //     static ISA::MroutineTableEntry buf[ISA::MroutineTableMaxEntryNum];
        //     assert(sizeof(buf) >= cacheLineSz);

        //     if(pkt->isError()) {
        //         panic("MENTER: memory fetch (%s) failed: %s", pkt->getAddrRange().to_string(), pkt->print());
        //     }

        //     getMemRawPtr(pkt, buf, cacheLineSz, traceData);

        //     const RegVal mbr = tc->readMetalMiscReg(metal_reg::MBR);

        //     Addr loadAddr;
        //     unsigned int count;
        //     this->calcLoadAddr(mbr, tc->getSystemPtr()->cacheLineSize(), this->imm, loadAddr, count);

        //     isa->loadMroutineTable(buf, count, (loadAddr - mbr) / (sizeof(ISA::MroutineTableEntry)));

        //     // lookup MRLB again
        //     const MRLBEntry &mrlbEnt = mrlb.get(this->imm);

        //     assert(&mrlbEnt != &MRLB::NullEntry);

        //     METAL_DBGPRINT(INSTS, MENTER, "MRLB *hit* for mroutine %d. Addr = 0x%lx, valid = %d.\n", this->imm, mrlbEnt.getAddr(), mrlbEnt.isValid());
        //     if (!mrlbEnt.isValid()) {
        //         return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
        //     } else {
        //         doMenter(xc->tcBase(), purifyTaggedAddr(mrlbEnt.getAddr(), xc->tcBase(), currEL(xc->tcBase()), true), *this);
        //     }

        //     return NoFault;
        // }
        
        // we know we will increase the metal level by 1 if successfully executed
        Fault Menter64::preExec(ExecContext *xc, trace::InstRecord *traceData) const
        {
            // set postExec based on preExec
            MetalInternalState post;
            post.set(xc->getPreExecMetalState());
            
            // exec state is the post state as metal registers are set in the new window
            xc->setExecMetalState(post);

            metal_reg::MSR_t msr = post.getMSR();
            msr.lv = msr.lv + 1;
            post.setMSR(msr);
            
            xc->setPostExecMetalState(post);

            return NoFault;
        }

        Fault Menter64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            ThreadContext * tc = xc->tcBase();
            ISA * isa = static_cast<ISA *>(tc->getIsaPtr());
            MRLB & mrlb = isa->getMrlbPtr();

            // lookup MRLB
            const MRLBEntry &mrlbEnt = mrlb.get(this->imm);

            assert(&mrlbEnt != &MRLB::NullEntry);

            METAL_DBGPRINT(INSTS, MENTER, "MRLB *hit* for mroutine %d. Addr = 0x%lx, valid = %d.\n", this->imm, mrlbEnt.getAddr(), mrlbEnt.isValid());
            if (!mrlbEnt.isValid()) {
                return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            } else {
                doMenter(xc, this, 
                    purifyTaggedAddr(mrlbEnt.getAddr(), xc->tcBase(), currEL(xc->tcBase()), true),  
                    xc->tcBase()->pcState().instAddr() + this->instSize(), 
                    0);
            }
            return NoFault;
        }
    }
}