#include "arch/arm/insts/metal.hh"
#include "arch/arm/regs/metal.hh"
#include "arch/arm/mlb.hh"
#include "arch/arm/isa.hh"
#include "arch/arm/pcstate.hh"
#include "arch/arm/table_walker.hh"
#include "arch/generic/memhelpers.hh"
#include "debug/Metal.hh"
#include "mem/packet_access.hh"
#include "mem/request.hh"

#include "base/cprintf.hh"

namespace gem5
{

    namespace ArmISA
    {

        std::string
        MetalImmOp8::generateDisassembly(
            Addr pc, const loader::SymbolTable *symtab) const
        {
            std::stringstream ss;
            ss << MetalDisasmPrefix;
            printMnemonic(ss, "", false);
            ss << (unsigned int)this->imm;
            return ss.str();
        }

        std::string
        MetalNakedOp::generateDisassembly(
            Addr pc, const loader::SymbolTable *symtab) const
        {
            std::stringstream ss;
            ss << MetalDisasmPrefix;
            printMnemonic(ss, "", false);
            return ss.str();
        }

        std::string
        MetalRegOp::generateDisassembly(
            Addr pc, const loader::SymbolTable *symtab) const
        {
            std::stringstream ss;
            ss << MetalDisasmPrefix;
            printMnemonic(ss, "", false);
            printMetalReg(ss, mReg);
            return ss.str();
        }

        std::string
        MetalRegOp2::generateDisassembly(
            Addr pc, const loader::SymbolTable *symtab) const
        {
            std::stringstream ss;
            ss << MetalDisasmPrefix;
            printMnemonic(ss, "", false);
            printMetalReg(ss, mReg);
            ccprintf(ss, ", ");
            printIntReg(ss, gReg);
            return ss.str();
        }

        std::string
        MetalRegOp3::generateDisassembly(
            Addr pc, const loader::SymbolTable *symtab) const
        {
            std::stringstream ss;
            ss << MetalDisasmPrefix;
            printMnemonic(ss, "", false);
            printMetalReg(ss, rl);
            ccprintf(ss, ", ");
            printIntReg(ss, rm);
            ccprintf(ss, ", ");
            printIntReg(ss, rn);
            return ss.str();
        }

        std::string MetalPMemRegOp::generateDisassembly(Addr pc, const loader::SymbolTable *symtab) const
        {
            std::stringstream ss;
            ss << MetalDisasmPrefix;
            printMnemonic(ss, "", false);
            printIntReg(ss, rl);
            ccprintf(ss, ", [");
            printIntReg(ss, rm);
            ccprintf(ss, ", ");
            printIntReg(ss, rn);
            ccprintf(ss, "]");
            return ss.str();
        }

        std::string MetalPMemRegImmOp::generateDisassembly(Addr pc, const loader::SymbolTable *symtab) const
        {
            std::stringstream ss;
            ss << MetalDisasmPrefix;
            printMnemonic(ss, "", false);
            printIntReg(ss, mReg);
            ccprintf(ss, ", ");
            switch(mode) {
                case Mode::NORMAL:
                case Mode::PREINDEX:
                    ccprintf(ss, "[");
                    printIntReg(ss, gReg);
                    ccprintf(ss, ", ");
                    ccprintf(ss, "#%#x]", imm);
                    if (mode == Mode::PREINDEX) {
                        ccprintf(ss, "!");
                    }
                    break;
                case Mode::POSTINDEX:
                    ccprintf(ss, "[");
                    printIntReg(ss, gReg);
                    ccprintf(ss, "], #%#x", imm);
                    break;
                default:
                    panic("Unknown Metal PMem mode: %d", static_cast<int>(this->mode));
            }
            return ss.str();
        }

        // menter
        Menter64::Menter64(ExtMachInst _machInst, uint8_t _imm) : MetalImmOp8("menter", _machInst, IntAluOp, _imm)
        {
            this->flags[IsCall] = true;
            this->flags[IsControl] = true;
            this->flags[IsDirectControl] = true;
            this->flags[IsInteger] = true;
            this->flags[IsUncondControl] = true;
            this->flags[IsLoad] = true;
        }

        void Menter64::doMenter(ThreadContext *tc, Addr npc, Addr lpc)
        {
            PCState pcState;
            set(pcState, tc->pcState());
            // set new PC
            pcState.instNPC(npc);
            tc->pcState(pcState);

            // increase metal level and switch to metal int bank
            metal_reg::MSR_t msr = tc->readMetalReg(metal_reg::MSR);
            msr.lv = msr.lv + 1;
            tc->setMetalReg(metal_reg::MSR, msr);

            // save link address
            tc->setMetalReg(metal_reg::MLR, lpc);

            METAL_DBGPRINT(INSTS, MENTER, "entering Metal mode: MBR = 0x%lx, npc = 0x%lx, MLR = 0x%lx.\n", tc->readMetalReg(metal_reg::MBR), npc, lpc);
        }

        void Menter64::doMenter(ThreadContext * tc, Addr npc, const ArmStaticInst &inst)
        {
            doMenter(tc, npc, tc->pcState().instAddr() + inst.instSize());
        }

        void Menter64::calcLoadAddr(Addr base, unsigned long align, unsigned int idx, Addr & _loadAddr, unsigned int & _count)
        {
            // ensure base is ISA::MroutineTableEntry aligned (enforced by WMR)
            assert((base & (sizeof(ISA::MroutineTableEntry) - 1)) == 0);
            // ensure align is power of 2
            assert(isPowerOf2(align));
            // ensure we can load something
            assert(sizeof(ISA::MroutineTableEntry) <= align);

            const Addr mroutineAddr = base + sizeof(ISA::MroutineTableEntry) * idx;
            const Addr loadAddr = mroutineAddr & (~(align - 1));
            assert(mroutineAddr >= loadAddr && mroutineAddr - loadAddr <= align);

            Addr nextAlign = loadAddr + align;
            // check that the entry is not crossing align boundry
            assert(mroutineAddr + sizeof(ISA::MroutineTableEntry) <= nextAlign);

            // bound check for the entire table
            const Addr limitAddr = base + sizeof(ISA::MroutineTableEntry) * ISA::MroutineTableMaxEntryNum;
            if (nextAlign > limitAddr) {
                nextAlign = limitAddr;
            }

            assert((nextAlign - loadAddr) % sizeof(ISA::MroutineTableEntry) == 0);
            _loadAddr = loadAddr;
            _count = (nextAlign - loadAddr) / sizeof(ISA::MroutineTableEntry);
        }

        Fault Menter64::initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const
        {
            ThreadContext * tc = xc->tcBase();
            ISA * isa = static_cast<ISA *>(tc->getIsaPtr());
            MRLB & mrlb = isa->getMrlbPtr();

            // lookup MRLB
            const MRLBEntry &mrlbEnt = mrlb.get(this->imm);

            Fault fault = NoFault;
            if (&mrlbEnt == &MRLB::NullEntry) {
                const RegVal mbr = xc->readMetalReg(metal_reg::MBR);

                // ensure mbr is 8 byte aligned (enforced by WMR)
                assert((mbr & (sizeof(ISA::MroutineTableEntry) - 1)) == 0);

                Addr loadAddr;
                unsigned int count;

                this->calcLoadAddr(mbr, tc->getSystemPtr()->cacheLineSize(), this->imm, loadAddr, count);

                METAL_DBGPRINT(INSTS, MENTER, "MRLB *miss* for mrouine %d. Mem load addr = 0x%lx, count = %d.\n", this->imm, loadAddr, count);

                fault = initiateMemRead(xc, loadAddr, tc->getSystemPtr()->cacheLineSize(), ArmISA::MMU::AllowUnaligned);
            } else {
                METAL_DBGPRINT(INSTS, MENTER, "MRLB *hit* for mroutine %d. Addr = 0x%lx, valid = %d.\n", this->imm, mrlbEnt.getAddr(), mrlbEnt.isValid());
                if (!mrlbEnt.isValid()) {
                    return std::make_shared<UndefinedInstruction>(machInst, true, mnemonic);
                } else {
                    doMenter(xc->tcBase(), purifyTaggedAddr(mrlbEnt.getAddr(), xc->tcBase(), currEL(xc->tcBase()), true), *this);
                }
            }

            return fault;
        }

        Fault Menter64::completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const
        {
            ThreadContext * tc = xc->tcBase();
            ISA * isa = static_cast<ISA *>(tc->getIsaPtr());
            MRLB & mrlb = isa->getMrlbPtr();
            const size_t cacheLineSz = tc->getSystemPtr()->cacheLineSize();

            static ISA::MroutineTableEntry buf[ISA::MroutineTableMaxEntryNum];
            assert(sizeof(buf) >= cacheLineSz);

            if(pkt->isError()) {
                panic("MENTER: memory fetch (%s) failed: %s", pkt->getAddrRange().to_string(), pkt->print());
            }

            getMemRawPtr(pkt, buf, cacheLineSz, traceData);

            const RegVal mbr = xc->readMetalReg(metal_reg::MBR);

            Addr loadAddr;
            unsigned int count;
            this->calcLoadAddr(mbr, tc->getSystemPtr()->cacheLineSize(), this->imm, loadAddr, count);

            isa->loadMroutineTable(buf, count, (loadAddr - mbr) / (sizeof(ISA::MroutineTableEntry)));

            // lookup MRLB again
            const MRLBEntry &mrlbEnt = mrlb.get(this->imm);

            assert(&mrlbEnt != &MRLB::NullEntry);

            METAL_DBGPRINT(INSTS, MENTER, "MRLB *hit* for mroutine %d. Addr = 0x%lx, valid = %d.\n", this->imm, mrlbEnt.getAddr(), mrlbEnt.isValid());
            if (!mrlbEnt.isValid()) {
                return std::make_shared<UndefinedInstruction>(machInst, true, mnemonic);
            } else {
                doMenter(xc->tcBase(), purifyTaggedAddr(mrlbEnt.getAddr(), xc->tcBase(), currEL(xc->tcBase()), true), *this);
            }

            return NoFault;
        }

        Fault Menter64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            // not maintaining this because we only need TimingSimpleCPU
            panic("unimplemented.");

            // ISA * isa = static_cast<ISA *>(xc->tcBase()->getIsaPtr());
            // metal_reg::MSR_t msr = xc->readMetalReg(metal_reg::MSR);

            // Addr npc;
            // if (!isa->lookupMroutineAddr(this->imm, npc) || !metal_reg::isMetalInitialized(msr)) {
            //     // mroutine does not exist or is invalid
            //     return std::make_shared<UndefinedInstruction>(machInst, true, mnemonic);
            // }
            // npc = purifyTaggedAddr(npc, xc->tcBase(), currEL(xc->tcBase()), true);

            // // set new PC
            // PCState pcState;
            // set(pcState, xc->pcState());
            // pcState.instNPC(npc);
            // xc->pcState(pcState);

            // // save link address
            // xc->setMetalReg(metal_reg::MLR, pcState.pc() + 4);
            // // increase metal level
            // msr = xc->readMetalReg(metal_reg::MSR);
            // msr.lv = msr.lv + 1;
            // xc->setMetalReg(metal_reg::MSR, msr);

            // return NoFault;
        }

        // mexit
        Mexit64::Mexit64(ExtMachInst _machInst, uint8_t _imm) : MetalImmOp8("mexit", _machInst, IntAluOp, _imm)
        {
            this->flags[IsControl] = true;
            this->flags[IsIndirectControl] = true;
            this->flags[IsInteger] = true;
            this->flags[IsReturn] = true;
            this->flags[IsUncondControl] = true;
        }

        Fault Mexit64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            // get mroutine table's base address
            const RegVal ret = xc->readMetalReg(metal_reg::MLR);
            metal_reg::MSR_t msr = xc->readMetalReg(metal_reg::MSR);
            const MexitFlags flags = static_cast<MexitFlags>(this->imm);

            // must be in Metal mode to mexit
            if (!metal_reg::isInMetalMode(msr)) {
                return std::make_shared<UndefinedInstruction>(machInst, true, mnemonic);
            }

            // set new PC
            const Addr target_addr = purifyTaggedAddr(ret, xc->tcBase(), currEL(xc->tcBase()), true);
            PCState pcState;
            set(pcState, xc->pcState());
            pcState.instNPC(target_addr);
            xc->pcState(pcState);

            CPSR newCpsr = 0;
            CPSR mspsr = 0;
            if (flags.rfi) {
                // restore PSTATE from MSPSR
                mspsr = xc->readMetalReg(metal_reg::MSPSR);
                const CPSR cpsr = xc->readMiscReg(MISCREG_CPSR);
                newCpsr = getPSTATEFromPSR(xc->tcBase(), cpsr, mspsr);
                // restore flags that are in separate regs
                xc->setMiscReg(MISCREG_NZCV, newCpsr);
                // restore other flags that are stored in CPSR
                xc->setMiscReg(MISCREG_CPSR, newCpsr);
            }

            METAL_DBGPRINT(INSTS, MEXIT, "exiting Metal mode: MLR = 0x%lx, flags = [rfi = %d (MSPSR = 0x%lx, NCPSR = 0x%lx), iim = %d]\n", ret, flags.rfi, mspsr, newCpsr, flags.iim);

            if (flags.iim) {
                msr.im = 1;
            }

            // decrease Metal level
            msr.lv = msr.lv - 1;
            xc->setMetalReg(metal_reg::MSR, msr);


            return NoFault;
        }

        // wmr
        Wmr64::Wmr64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg) : MetalRegOp2("wmr", _machInst, IntAluOp, _mreg, _greg)
        {
            setSrcRegIdx(_numSrcRegs++, gem5::ArmISA::couldBeZero(gReg) ? RegId() : intRegClass[gReg]);
            setDestRegIdx(_numDestRegs++, metalRegClass[mReg]);
            _numTypedDestRegs[metalRegClass.type()]++;
            this->flags[IsMacroop] = true;
            this->flags[IsInteger] = true;
            StaticInstPtr uop;

            uop = new Wmr64_u(_machInst, _opClass, mReg, gReg);
            this->addMicroOps(uop);
            if (mReg == metal_reg::MIB) {
                this->flags[IsLoad] = true;
                for (int i = 0; i < ISA::InstInterceptTableTotalSize / ISA::InstInterceptTableLoadSize; i++) {
                    uop = new Mliit64_u(_machInst, _opClass, i * ISA::InstInterceptTableLoadSize, ISA::InstInterceptTableLoadSize);
                    this->addMicroOps(uop);
                }
            } else if (mReg == metal_reg::MEB) {
                this->flags[IsLoad] = true;
                for (int i = 0; i < ISA::ExcInterceptTableTotalSize / ISA::ExcInterceptTableLoadSize; i++) {
                    uop = new Mleit64_u(_machInst, _opClass, i * ISA::ExcInterceptTableLoadSize, ISA::ExcInterceptTableLoadSize);
                    this->addMicroOps(uop);
                }
            }
            this->finalizeMicroOps();
        }

        Fault Wmr64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            panic("unimplemented");
        }

        // rmr
        Rmr64::Rmr64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg) : MetalRegOp2("rmr", _machInst, IntAluOp, _mreg, _greg)
        {
            setDestRegIdx(_numDestRegs++, gem5::ArmISA::couldBeZero(gReg) ? RegId() : intRegClass[gReg]);
            setSrcRegIdx(_numSrcRegs++, metalRegClass[mReg]);
            _numTypedDestRegs[intRegClass.type()]++;

            this->flags[IsInteger] = true;
        }

        Fault Rmr64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            metal_reg::MSR_t msr = xc->readMetalReg(metal_reg::MSR);
            RegVal v = xc->readMetalReg(mReg);

            METAL_DBGPRINT(INSTS, RMR, "mReg = %s (0x%lx), gReg = %d.\n", printMetalReg(mReg), v, gReg);

            if (!metal_reg::canReadMetalReg(msr, mReg))
            {
                return std::make_shared<UndefinedInstruction>(machInst, true, mnemonic);
            }

            xc->setRegOperand(this, 0, v);

            return NoFault;
        }

        // rar64
        Rar64::Rar64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg) : MetalRegOp2("rar", _machInst, IntAluOp, _mreg, _greg)
        {
            setSrcRegIdx(_numSrcRegs++, metalRegClass[mReg]);
            setDestRegIdx(_numDestRegs++, metalRegClass[gReg]);
            _numTypedDestRegs[metalRegClass.type()]++;

            this->flags[IsInteger] = true;
        }

        Fault Rar64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            metal_reg::MSR_t msr = xc->readMetalReg(metal_reg::MSR);
            RegVal idx = xc->readMetalReg(this->mReg);

            METAL_DBGPRINT(INSTS, RAR, "idxMReg = %s, srcGReg = %d, dstMReg = %s.\n", printMetalReg(this->mReg), idx, printMetalReg(this->gReg));

            if (!metal_reg::canWriteMetalReg(msr, this->gReg) || !metal_reg::canReadMetalReg(msr, this->mReg)
                || idx >= int_reg::NumArchRegs) {
                return std::make_shared<UndefinedInstruction>(machInst, true, mnemonic);
            }

            RegVal v = xc->getReg(intRegClass[idx]);
            xc->setMetalReg(this->gReg, v);

            return NoFault;
        }

        // war64
        War64::War64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg) : MetalRegOp2("war", _machInst, IntAluOp, _mreg, _greg)
        {
            setSrcRegIdx(_numSrcRegs++, metalRegClass[mReg]);
            setSrcRegIdx(_numDestRegs++, metalRegClass[gReg]);\
            // writing to int class
            _numTypedDestRegs[intRegClass.type()]++;

            this->flags[IsInteger] = true;
        }

        Fault War64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            metal_reg::MSR_t msr = xc->readMetalReg(metal_reg::MSR);
            RegVal idx = xc->readMetalReg(this->mReg);

            METAL_DBGPRINT(INSTS, WAR, "idxMReg = %s, dstGReg = %d, srcMReg = %s.\n", printMetalReg(this->mReg), idx, printMetalReg(this->gReg));

            if (!metal_reg::canWriteMetalReg(msr, this->gReg) || !metal_reg::canReadMetalReg(msr, this->mReg)
                || idx >= int_reg::NumArchRegs) {
                return std::make_shared<UndefinedInstruction>(machInst, true, mnemonic);
            }

            RegVal v = xc->readMetalReg(this->gReg);
            xc->setReg(intRegClass[idx], v);

            return NoFault;
        }

        // rpr64
        Rpr64::Rpr64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg) : MetalRegOp2("rpr", _machInst, IntAluOp, _mreg, _greg)
        {
            setSrcRegIdx(_numSrcRegs++, metalRegClass[mReg]);
            setSrcRegIdx(_numSrcRegs++, metalRegClass[gReg]);
            // writing to metal class
            _numTypedDestRegs[metalRegClass.type()]++;

            this->flags[IsInteger] = true;
        }

        Fault Rpr64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            metal_reg::MSR_t msr = xc->readMetalReg(metal_reg::MSR);
            RegVal idx = xc->readMetalReg(this->mReg);
            ISA * isa = static_cast<ISA *>(xc->tcBase()->getIsaPtr());

            METAL_DBGPRINT(INSTS, RPR, "idxMReg = %s, srcGReg = %d, dstMReg = %s.\n", printMetalReg(this->mReg), idx, printMetalReg(this->gReg));

            if (!metal_reg::canWriteMetalReg(msr, this->gReg)
                || !metal_reg::canReadMetalReg(msr, this->mReg)
                || idx >= int_reg::NumArchRegs
                || (!metal_reg::isPrivilegeCheckDisabled(msr) && !metal_reg::isInMetalMode(msr)) ) {
                return std::make_shared<UndefinedInstruction>(machInst, true, mnemonic);
            }

            RegVal v = isa->readPrevIntReg(idx);
            xc->setMetalReg(gReg, v);

            return NoFault;
        }

        // wpr64
        Wpr64::Wpr64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg) : MetalRegOp2("wpr", _machInst, IntAluOp, _mreg, _greg)
        {
            setSrcRegIdx(_numSrcRegs++, metalRegClass[mReg]);
            setSrcRegIdx(_numDestRegs++, metalRegClass[gReg]);\
            // writing to int class
            _numTypedDestRegs[intRegClass.type()]++;

            this->flags[IsInteger] = true;
        }

        Fault Wpr64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            metal_reg::MSR_t msr = xc->readMetalReg(metal_reg::MSR);
            RegVal idx = xc->readMetalReg(this->mReg);
            ISA * isa = static_cast<ISA *>(xc->tcBase()->getIsaPtr());

            METAL_DBGPRINT(INSTS, WPR, "idxMReg = %s, dstGReg = %d, srcMReg = %s.\n", printMetalReg(this->mReg), idx, printMetalReg(this->gReg));

            if (!metal_reg::canWriteMetalReg(msr, this->gReg)
                || !metal_reg::canReadMetalReg(msr, this->mReg)
                || idx >= int_reg::NumArchRegs
                || (!metal_reg::isPrivilegeCheckDisabled(msr) && !metal_reg::isInMetalMode(msr))) {
                return std::make_shared<UndefinedInstruction>(machInst, true, mnemonic);
            }

            RegVal v = xc->readMetalReg(this->gReg);
            isa->setPrevIntReg(idx, v);

            return NoFault;
        }

        // rtlb
        Rtlb64::Rtlb64(ExtMachInst _machInst, RegIndex _rl, RegIndex _rm,
                       RegIndex _rn)
                       : MetalRegOp3("rtlb", _machInst, IntAluOp, _rl, _rm,
                                         _rn)
        {
            this->flags[IsInteger] = true;
        }

        Fault Rtlb64::execute(ExecContext *xc, trace::InstRecord *traceData)
            const
        {
            metal_reg::MSR_t msr = xc->readMetalReg(metal_reg::MSR);

            METAL_DBGPRINT(INSTS, RTLB, "RTLB: rl = %s, rm = %s, rn = %s\n", printMetalReg(rl), printMetalReg(rm), printMetalReg(rn));

            if (!metal_reg::canWriteMetalReg(msr, rl)
                || !metal_reg::canWriteMetalReg(msr, rm)
                || !metal_reg::canReadMetalReg(msr, rn)
                || (!metal_reg::isPrivilegeCheckDisabled(msr) && !metal_reg::isInMetalMode(msr)))
            {
                return std::make_shared<UndefinedInstruction>(machInst, false, mnemonic);
            }

            TableWalker::LongDescriptor ld;
            ld.aarch64 = true;

            TLBEntryInfo tei;

            RegVal vaddr = xc->readMetalReg(rn);
            Addr va = vaddr;

            ArmISA::TlbEntry *te =
                dynamic_cast<ArmISA::TLB *>(
                    xc->tcBase()->getMMUPtr()->dtb)
                    ->getEntry(va);

            assert(te);

            // Create and fill a new page table entry

            tei.set_isHyp(te->isHyp);
            tei.set_asid(te->asid);
            tei.set_vmid(te->vmid);
            // ld.data = insertBits(ld.data, )
            switch (te->N) {
                // type == Page
                case Grain4KB:
                case Grain16KB:
                case Grain64KB:
                    ld.data = insertBits(ld.data, 1, 0, 0x3);
                    ld.grainSize = (GrainSize)te->N;
                    break;

                // type == Block
                case 21:    // 2 MiB
                case 30:    // 1 GiB
                    ld.data = insertBits(ld.data, 1, 0, 0x1);
                    ld.grainSize = Grain4KB;
                    break;
                case 25:    // 32 MiB
                    ld.data = insertBits(ld.data, 1, 0, 0x1);
                    ld.grainSize = Grain16KB;
                    break;
                case 29:    // 256 MiB
                case 42:    // 4 TiB
                    ld.data = insertBits(ld.data, 1, 0, 0x1);
                    ld.grainSize = Grain64KB;
                    break;
                default:
                    // panix - bad descriptor?
                    break;
            }

            // assert(ld.offsetBits() == te->N);

            va              = te->vpn << te->N;
            // pfn
            ld.data = insertBits(ld.data, 47, te->N, bits(te->pfn << te->N, 47,
                                 te->N));
            if (te->N == 16)
                ld.data = insertBits(ld.data, 15, 12, bits(te->pfn << te->N,
                                     51, 48)); // 64k pages
            // domain always TlbEntry::DomainType::Client for LongDescriptor
            // te.domain         = ld.domain();
            ld.lookupLevel  = te->lookupLevel;
            ld.data = insertBits(ld.data, 5, te->ns);
            tei.set_isSecure(!te->nstid);
            // xn
            ld.data = insertBits(ld.data, 54, te->xn);
            tei.set_type(te->type == TypeTLB::instruction ? true : false);
            tei.set_el(te->el);
            // ld.global()
            ld.data = insertBits(ld.data, 11, !te->global);
            // ld.pxn()
            ld.data = insertBits(ld.data, 53, te->pxn);
            // ld.ap()
            ld.data = insertBits(ld.data, 7, 6, te->ap);
            tei.set_mtype(te->mtype);
            tei.set_nc(te->nonCacheable);
            // Attributes formatted according to the 64-bit PAR
            tei.set_attr(te->attributes >> 56);
            // ld.sh()
            ld.data = insertBits(ld.data, 9, 8, (te->attributes >> 7) & 0b11);

            tei.set_ao(te->attrOverride);
            tei.set_ai(te->access);

            xc->setMetalReg(rl, (RegVal)ld.data);
            xc->setMetalReg(rm, (RegVal)tei.data);

            return NoFault;
        }

        // wtlb
        Wtlb64::Wtlb64(ExtMachInst _machInst, RegIndex _rl, RegIndex _rm,
                       RegIndex _rn) : MetalRegOp3("wtlb", _machInst,
                                                       IntAluOp, _rl, _rm, _rn)
        {
            this->flags[IsInteger] = true;
        }

        Fault Wtlb64::execute(ExecContext *xc, trace::InstRecord *traceData)
            const
        {
            metal_reg::MSR_t msr = xc->readMetalReg(metal_reg::MSR);

            METAL_DBGPRINT(INSTS, WTLB, "WTLB: rl = %s, rm = %s, rn = %s\n", printMetalReg(rl), printMetalReg(rm), printMetalReg(rn));

            if (!metal_reg::canReadMetalReg(msr, rl)
                || !metal_reg::canReadMetalReg(msr, rm)
                || !metal_reg::canReadMetalReg(msr, rn)
                || (!metal_reg::isPrivilegeCheckDisabled(msr) && !metal_reg::isInMetalMode(msr)) )
            {
                return std::make_shared<UndefinedInstruction>(machInst, false, mnemonic);
            }

            RegVal desc = xc->readMetalReg(rl);
            RegVal info = xc->readMetalReg(rm);
            RegVal vaddr = xc->readMetalReg(rn);

            TableWalker::LongDescriptor ld;
            ld.data = desc;
            ld.aarch64 = true;
            ld.lookupLevel = enums::ArmLookupLevel::L3;

            TLBEntryInfo tei;
            tei.data = info;

            Addr va = vaddr;

            TlbEntry te;

            // Create and fill a new page table entry
            te.valid          = true;
            te.longDescFormat = true;
            te.isHyp          = tei.isHyp();
            te.asid           = tei.asid();
            te.vmid           = tei.vmid();
            te.N              = ld.offsetBits();
            te.vpn            = va >> te.N;
            te.size           = (1<<te.N) - 1;
            te.pfn            = ld.pfn();
            te.domain         = ld.domain();
            te.lookupLevel    = ld.lookupLevel;
            te.ns             = bits(ld.data, 5);
            te.nstid          = !tei.isSecure();
            te.xn             = ld.xn();
            te.type           = tei.type() ?
                TypeTLB::instruction : TypeTLB::data;
            te.el             = tei.el();
            te.global         = !bits(ld.data, 11);
            te.pxn = ld.pxn();
            te.ap = ld.ap();
            te.mtype = tei.mtype();
            te.nonCacheable = tei.nc();
            te.shareable       = ld.sh() == 2;
            te.outerShareable = (ld.sh() & 0x2) ? true : false;
            // Attributes formatted according to the 64-bit PAR
            te.attributes = ((uint64_t)tei.attr() << 56) |
                (1 << 11) |     // LPAE bit
                (te.ns << 9) |  // NS bit
                (ld.sh() << 7);

            te.attrOverride = tei.ao();
            te.access       = tei.ai();

            if (tei.itb())
                dynamic_cast<ArmISA::TLB *>(xc->tcBase()->getMMUPtr()->itb)
                    ->insert(te);
            else
                dynamic_cast<ArmISA::TLB *>(xc->tcBase()->getMMUPtr()->dtb)
                    ->insert(te);

            return NoFault;
        }
        // mcli
        Mcli64::Mcli64(ExtMachInst _machInst) : MetalNakedOp("mcli", _machInst, IntAluOp)
        {
            this->flags[IsInteger] = true;
        }

        Fault Mcli64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            metal_reg::MSR_t msr = xc->readMetalReg(metal_reg::MSR);

            METAL_DBGPRINT(INSTS, MCLI, "Masking instruction intercept.\n");

            if (!metal_reg::canWriteMetalReg(msr, metal_reg::MSR)) {
                return std::make_shared<UndefinedInstruction>(machInst, true, mnemonic);
            }

            msr.ii = 0;
            xc->setMetalReg(metal_reg::MSR, msr);
            return NoFault;
        }

        // msti
        Msti64::Msti64(ExtMachInst _machInst) : MetalNakedOp("msti", _machInst, IntAluOp)
        {
            this->flags[IsInteger] = true;
        }

        Fault Msti64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            metal_reg::MSR_t msr = xc->readMetalReg(metal_reg::MSR);

            METAL_DBGPRINT(INSTS, MSTI, "Setting instruction intercept.\n");

            if (!metal_reg::canWriteMetalReg(msr, metal_reg::MSR)) {
                return std::make_shared<UndefinedInstruction>(machInst, true, mnemonic);
            }

            msr.ii = 1;
            xc->setMetalReg(metal_reg::MSR, msr);
            return NoFault;
        }

        // Mliit64_u
        Mliit64_u::Mliit64_u(ExtMachInst _machInst, OpClass __opClass, uint32_t _offset, uint32_t _size) :
            MetalNakedOp("mliit_u", _machInst, __opClass), offset(_offset), size(_size)
        {
            this->flags[IsMicroop] = true;
            this->flags[IsLoad] = true;
            this->flags[IsInteger] = true;
        }

        Fault Mliit64_u::initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const
        {
            ThreadContext *tc = xc->tcBase();
            metal_reg::MSR_t msr = xc->readMetalReg(metal_reg::MSR);
            RegVal mib = xc->readMetalReg(metal_reg::MIB);

            METAL_DBGPRINT(INSTS, MLIIT_U, "Loading inst intercept table at 0x%lx + 0x%lx, size %u.\n", mib, this->offset, this->size);

            if (!metal_reg::canReadMetalReg(msr, metal_reg::MIB)) {
                return std::make_shared<UndefinedInstruction>(machInst, true, mnemonic);
            }

            Fault fault = initiateMemRead(xc, purifyTaggedAddr(this->offset + mib, tc, currEL(tc), true), this->size, ArmISA::MMU::AllowUnaligned);

            return NoFault;
        }

        Fault Mliit64_u::completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const
        {
            ThreadContext *tc = xc->tcBase();
            ISA * isa = static_cast<ISA *>(tc->getIsaPtr());
            RegVal mib = xc->readMetalReg(metal_reg::MIB);

            if (pkt->isError()) {
                panic("Data fetch failed.");
            }

            static char buf[ISA::InstInterceptTableLoadSize];
            assert(this->size <= ISA::InstInterceptTableLoadSize);
            getMemRawPtr(pkt, buf, this->size, traceData);
            isa->loadInstInterceptTable(buf, purifyTaggedAddr(this->offset + mib, tc, currEL(tc), true), this->size);
            return NoFault;
        }

        Fault Mliit64_u::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            panic("unimplemented");
        }

        std::string
        Mliit64_u::generateDisassembly(
            Addr pc, const loader::SymbolTable *symtab) const
        {
            std::stringstream ss;
            ss << MetalDisasmPrefix;
            printMnemonic(ss, "", false);
            ccprintf(ss, "0x%x, 0x%x", this->offset, this->size);
            return ss.str();
        }

        // Mleit64_u
        Mleit64_u::Mleit64_u(ExtMachInst _machInst, OpClass __opClass, uint32_t _offset, uint32_t _size) :
            MetalNakedOp("mleit_u", _machInst, __opClass), offset(_offset), size(_size)
        {
            this->flags[IsMicroop] = true;
            this->flags[IsLoad] = true;
            this->flags[IsInteger] = true;
        }

        Fault Mleit64_u::initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const
        {
            ThreadContext *tc = xc->tcBase();
            metal_reg::MSR_t msr = xc->readMetalReg(metal_reg::MSR);
            RegVal meb = xc->readMetalReg(metal_reg::MEB);

            METAL_DBGPRINT(INSTS, MLEIT_U, "Loading exc intercept table at 0x%lx + 0x%lx, size %u.\n", meb, this->offset, this->size);

            if (!metal_reg::canReadMetalReg(msr, metal_reg::MEB)) {
                return std::make_shared<UndefinedInstruction>(machInst, true, mnemonic);
            }

            Fault fault = initiateMemRead(xc, purifyTaggedAddr(this->offset + meb, tc, currEL(tc), true), this->size, ArmISA::MMU::AllowUnaligned);

            return NoFault;
        }

        Fault Mleit64_u::completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const
        {
            ThreadContext *tc = xc->tcBase();
            ISA * isa = static_cast<ISA *>(tc->getIsaPtr());
            RegVal meb = xc->readMetalReg(metal_reg::MEB);

            if (pkt->isError()) {
                panic("Data fetch failed.");
            }

            static char buf[ISA::ExcInterceptTableLoadSize];
            assert(this->size <= ISA::ExcInterceptTableLoadSize);
            getMemRawPtr(pkt, buf, this->size, traceData);
            isa->loadExcInterceptTable(buf, purifyTaggedAddr(this->offset + meb, tc, currEL(tc), true), this->size);
            return NoFault;
        }

        Fault Mleit64_u::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            panic("unimplemented");
        }

        std::string
        Mleit64_u::generateDisassembly(
            Addr pc, const loader::SymbolTable *symtab) const
        {
            std::stringstream ss;
            ss << MetalDisasmPrefix;
            printMnemonic(ss, "", false);
            ccprintf(ss, "0x%x, 0x%x", this->offset, this->size);
            return ss.str();
        }

        // Wmr64_u
        Wmr64_u::Wmr64_u(ExtMachInst _machInst, OpClass __opClass, RegIndex _mReg, RegIndex _gReg) :
            MetalRegOp2("wmr64_u", _machInst, __opClass, _mReg, _gReg)
        {
            setSrcRegIdx(_numSrcRegs++, gem5::ArmISA::couldBeZero(gReg) ? RegId() : intRegClass[gReg]);
            setDestRegIdx(_numDestRegs++, metalRegClass[mReg]);

            _numTypedDestRegs[metalRegClass.type()]++;

            this->flags[IsMicroop] = true;
            this->flags[IsInteger] = true;
        }

        Fault Wmr64_u::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            metal_reg::MSR_t msr = xc->readMetalReg(metal_reg::MSR);
            RegVal v = xc->getRegOperand(this, 0);

            METAL_DBGPRINT(INSTS, WMR_U, "mReg = %s, gReg = %d (0x%lx).\n", printMetalReg(mReg), gReg, v);

            bool allowWriting = false;
            // allow writing to Metal Base Register outside of Metal mode to initialize Metal
            if (!isMetalInitialized(msr)) {
                if (this->mReg == metal_reg::MBR) {
                    // set init bit
                    msr.init = 1;
                    xc->setMetalReg(metal_reg::MSR, msr);
                    allowWriting = true;
                }
            } else {
                allowWriting = metal_reg::canWriteMetalReg(msr, mReg);
            }

            if (!allowWriting) {
                return std::make_shared<UndefinedInstruction>(machInst, true, mnemonic);
            }

            Fault fault = NoFault;
            switch(this->mReg) {
                case metal_reg::MSR: {
                    metal_reg::MSR_t new_val = v;
                    // msr.init is readonly
                    new_val.init = msr.init;
                    // msr.lv is readonly
                    new_val.lv = msr.lv;
                    v = new_val;
                    break;
                }
                default: {
                    break;
                }
            }

            if (fault == NoFault) {
                xc->setMetalReg(mReg, v);
            }

            return fault;
        }

        // pldri
        template <typename T>
        Pldri<T>::Pldri(ExtMachInst _machInst, RegIndex _dReg, RegIndex _sReg, int32_t _imm, Mode _mode) :
            MetalPMemRegImmOp("pldr", _machInst, MemReadOp, _dReg, _sReg, _imm, _mode)
        {
            setSrcRegIdx(_numSrcRegs++, intRegClass[_sReg]);
            setDestRegIdx(_numDestRegs++, intRegClass[_dReg]);
            setDestRegIdx(_numDestRegs++, intRegClass[_sReg]);
            _numTypedDestRegs[intRegClass.type()] += 2;

            this->flags[IsInteger] = true;
            this->flags[IsLoad] = true;
        }

        template <typename T>
        Fault Pldri<T>::initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const
        {
            metal_reg::MSR_t msr = xc->readMetalReg(metal_reg::MSR);
            Addr base = xc->getRegOperand(this, 0);

            METAL_DBGPRINT(INSTS, PLDRI, "dReg = %u, sReg = %u, imm = %d, mode = %#x, size = %u.\n",
                    mReg, gReg, imm,
                    static_cast<int>(mode) ,
                    sizeof(T));

            if (!metal_reg::isPrivilegeCheckDisabled(msr) && !metal_reg::isInMetalMode(msr)) {
                // only available in Metal mode
                return std::make_shared<UndefinedInstruction>(machInst, true, mnemonic);
            }

            switch (mode) {
                case Mode::PREINDEX:
                    base = base + imm;
                    xc->setRegOperand(this, 1, base);
                    break;
                case Mode::NORMAL:
                    base = base + imm;
                    break;
                default:
                    break;
            }

            Fault fault = NoFault;

            fault = initiateMemRead(xc, base, sizeof(T), ArmISA::MMU::AllowUnaligned | ArmISA::MMU::BypassMMU);

            return fault;
        }

        template <typename T>
        Fault Pldri<T>::completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const
        {
            assert(metal_reg::isPrivilegeCheckDisabled(xc->readMetalReg(metal_reg::MSR))
                    || metal_reg::isInMetalMode(xc->readMetalReg(metal_reg::MSR)));

            if (pkt->isError()) {
                panic("Data fetch failed.");
            }

            T mem;
            if (isBigEndian64(xc->tcBase())) {
                getMem<ByteOrder::big, T>(pkt, mem, traceData);
            } else {
                getMem<ByteOrder::little, T>(pkt, mem, traceData);
            }

            xc->setRegOperand(this, 0, static_cast<RegVal>(mem));

            if (mode == Mode::POSTINDEX) {
                xc->setRegOperand(this, 1, xc->getRegOperand(this, 0) + imm);
            }

            return NoFault;
        }

        template <typename T>
        Fault Pldri<T>::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            panic("unimplemented.");
        }

        // pstri
        template <typename T>
        Pstri<T>::Pstri(ExtMachInst _machInst, RegIndex _sReg, RegIndex _aReg, int32_t _imm, Mode _mode) :
            MetalPMemRegImmOp("pstr", _machInst, MemWriteOp, _sReg, _aReg, _imm, _mode)
        {
            setSrcRegIdx(_numSrcRegs++, intRegClass[_sReg]);
            setSrcRegIdx(_numSrcRegs++, intRegClass[_aReg]);
            setDestRegIdx(_numDestRegs++,intRegClass[_aReg]);
            _numTypedDestRegs[intRegClass.type()]++;

            this->flags[IsInteger] = true;
            this->flags[IsStore] = true;
        }

        template <typename T>
        Fault Pstri<T>::initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const
        {
            metal_reg::MSR_t msr = xc->readMetalReg(metal_reg::MSR);

            METAL_DBGPRINT(INSTS, PSTRI, "sReg = %u, aReg = %u, imm = %d, mode = %#x, size = %u.\n",
                    mReg, gReg, imm,
                    static_cast<int>(mode) ,
                    sizeof(T));

            if (!metal_reg::isPrivilegeCheckDisabled(msr) && !metal_reg::isInMetalMode(msr)) {
                // only available in Metal mode
                return std::make_shared<UndefinedInstruction>(machInst, true, mnemonic);
            }

            Addr base = xc->getRegOperand(this, 1);

            switch (mode) {
                case Mode::PREINDEX:
                    base = base + imm;
                    xc->setRegOperand(this, 0, base);
                    break;
                case Mode::NORMAL:
                    base = base + imm;
                    break;
                default:
                    break;
            }

            Fault fault = NoFault;

            T mem = static_cast<T>(xc->getRegOperand(this, 0));

            if (isBigEndian64(xc->tcBase())) {
                fault = writeMemTimingBE(xc, traceData, mem, base, ArmISA::MMU::AllowUnaligned | ArmISA::MMU::BypassMMU, nullptr);
            } else {
                fault = writeMemTimingLE(xc, traceData, mem, base, ArmISA::MMU::AllowUnaligned | ArmISA::MMU::BypassMMU, nullptr);
            }

            return fault;
        }

        template <typename T>
        Fault Pstri<T>::completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const
        {
            assert(metal_reg::isPrivilegeCheckDisabled(xc->readMetalReg(metal_reg::MSR))
                || metal_reg::isInMetalMode(xc->readMetalReg(metal_reg::MSR)));

            if (pkt->isError()) {
                panic("Data write failed.");
            }

            return NoFault;
        }

        template <typename T>
        Fault Pstri<T>::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            panic("unimplemented.");
        }


        // pldrr
        template <typename T>
        Pldrr<T>::Pldrr(ExtMachInst _machInst, RegIndex _dReg, RegIndex _bReg, RegIndex _oReg) :
            MetalPMemRegOp("pldr", _machInst, MemReadOp, _dReg, _bReg, _oReg)
        {
            setSrcRegIdx(_numSrcRegs++, intRegClass[_bReg]);
            setSrcRegIdx(_numSrcRegs++, intRegClass[_oReg]);
            setDestRegIdx(_numDestRegs++, intRegClass[_dReg]);
            _numTypedDestRegs[intRegClass.type()]++;

            this->flags[IsInteger] = true;
            this->flags[IsLoad] = true;
        }

        template <typename T>
        Fault Pldrr<T>::initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const
        {
            metal_reg::MSR_t msr = xc->readMetalReg(metal_reg::MSR);
            const Addr addr = xc->getRegOperand(this, 0) + xc->getRegOperand(this, 1);

            METAL_DBGPRINT(INSTS, PLDRR, "dReg = %u, bReg = %u, oReg = %u, addr = %#lx, size = %u.\n",
                    rl, rm, rn, addr,
                    sizeof(T));

            if (!metal_reg::isPrivilegeCheckDisabled(msr) && !metal_reg::isInMetalMode(msr)) {
                // only available in Metal mode
                return std::make_shared<UndefinedInstruction>(machInst, true, mnemonic);
            }


            Fault fault = initiateMemRead(xc, addr, sizeof(T), ArmISA::MMU::AllowUnaligned | ArmISA::MMU::BypassMMU);

            return fault;
        }

        template <typename T>
        Fault Pldrr<T>::completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const
        {
            assert(metal_reg::isPrivilegeCheckDisabled(xc->readMetalReg(metal_reg::MSR)) ||
                metal_reg::isInMetalMode(xc->readMetalReg(metal_reg::MSR)));

            if (pkt->isError()) {
                panic("Data fetch failed.");
            }

            T mem;
            if (isBigEndian64(xc->tcBase())) {
                getMem<ByteOrder::big, T>(pkt, mem, traceData);
            } else {
                getMem<ByteOrder::little, T>(pkt, mem, traceData);
            }

            xc->setRegOperand(this, 0, static_cast<RegVal>(mem));

            return NoFault;
        }

        template <typename T>
        Fault Pldrr<T>::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            panic("unimplemented.");
        }


        // pstrr
        template <typename T>
        Pstrr<T>::Pstrr(ExtMachInst _machInst, RegIndex _dReg, RegIndex _bReg, RegIndex _oReg) :
            MetalPMemRegOp("pstr", _machInst, MemWriteOp, _dReg, _bReg, _oReg)
        {
            setSrcRegIdx(_numSrcRegs++, intRegClass[_dReg]);
            setSrcRegIdx(_numSrcRegs++, intRegClass[_bReg]);
            setSrcRegIdx(_numSrcRegs++, intRegClass[_oReg]);

            this->flags[IsInteger] = true;
            this->flags[IsStore] = true;
        }

        template <typename T>
        Fault Pstrr<T>::initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const
        {
            metal_reg::MSR_t msr = xc->readMetalReg(metal_reg::MSR);
            const Addr addr = xc->getRegOperand(this, 1) + xc->getRegOperand(this, 2);

            METAL_DBGPRINT(INSTS, PSTRR, "sReg = %u, bReg = %u, oReg = %u, addr = %#lx, size = %u.\n",
                    rl, rm, rn, addr,
                    sizeof(T));

            if (!metal_reg::isPrivilegeCheckDisabled(msr) && !metal_reg::isInMetalMode(msr)) {
                // only available in Metal mode
                return std::make_shared<UndefinedInstruction>(machInst, true, mnemonic);
            }

            Fault fault = NoFault;

            T mem = static_cast<T>(xc->getRegOperand(this, 0));

            if (isBigEndian64(xc->tcBase())) {
                fault = writeMemTimingBE(xc, traceData, mem, addr, ArmISA::MMU::AllowUnaligned | ArmISA::MMU::BypassMMU, nullptr);
            } else {
                fault = writeMemTimingLE(xc, traceData, mem, addr, ArmISA::MMU::AllowUnaligned | ArmISA::MMU::BypassMMU, nullptr);
            }

            return fault;
        }

        template <typename T>
        Fault Pstrr<T>::completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const
        {
            assert(metal_reg::isPrivilegeCheckDisabled(xc->readMetalReg(metal_reg::MSR)) ||
                    metal_reg::isInMetalMode(xc->readMetalReg(metal_reg::MSR)));

            if (pkt->isError()) {
                panic("Data fetch failed.");
            }

            return NoFault;
        }

        template <typename T>
        Fault Pstrr<T>::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            panic("unimplemented.");
        }
    } // namespace ArmISA
} // namespace gem5
