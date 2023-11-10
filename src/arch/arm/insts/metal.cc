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
            ss << this->imm;
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
            ccprintf(ss, ", ");
            printIntReg(ss, gReg);
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

        void Menter64::doMenter(ExecContext *xc, Addr addr)
        {
            PCState pcState;
            set(pcState, xc->pcState());
    
            // save link address
            xc->setMetalReg(metal_reg::MLR, pcState.pc() + 4);

            // set new PC
            pcState.instNPC(addr);
            xc->pcState(pcState);

            // increase metal level
            metal_reg::MSR_t msr = xc->readMetalReg(metal_reg::MSR);
            msr.lv = msr.lv + 1;
            xc->setMetalReg(metal_reg::MSR, msr);
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

            DPRINTF(Metal, "MENTER: MBR = 0x%lx, mroutine = %d\n", xc->readMetalReg(metal_reg::MBR), this->imm);

            // lookup MRLB
            const MRLBEntry &mrlbEnt = mrlb.get(this->imm);
            
            Fault fault = NoFault;
            if (&mrlbEnt == &MRLB::NullMRLBEntry) {
                const RegVal mbr = xc->readMetalReg(metal_reg::MBR);

                // ensure mbr is 8 byte aligned (enforced by WMR)
                assert((mbr & (sizeof(ISA::MroutineTableEntry) - 1)) == 0);

                Addr loadAddr;
                unsigned int count;

                this->calcLoadAddr(mbr, tc->getSystemPtr()->cacheLineSize(), this->imm, loadAddr, count);

                DPRINTF(Metal, "MENTER: MRLB *miss* for mrouine %d. Mem load addr = 0x%lx, count = %d.\n", this->imm, loadAddr, count);

                fault = initiateMemRead(xc, loadAddr, tc->getSystemPtr()->cacheLineSize(), ArmISA::MMU::AllowUnaligned);
            } else {
                //DPRINTF(Metal, "MENTER: MRLB *hit* for mroutine %d. Addr = 0x%lx, valid = %d.\n", this->imm, mrlbEnt.getAddr(), mrlbEnt.isValid());
                if (!mrlbEnt.isValid()) {
                    return std::make_shared<UndefinedInstruction>(machInst, true, mnemonic);
                } else {
                    this->doMenter(xc, purifyTaggedAddr(mrlbEnt.getAddr(), xc->tcBase(), currEL(xc->tcBase()), true));
                }
            }
    
            return fault;
        }

        Fault Menter64::completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const
        {
            ThreadContext * tc = xc->tcBase();
            ISA * isa = static_cast<ISA *>(tc->getIsaPtr());
            const size_t cacheLineSz = tc->getSystemPtr()->cacheLineSize();

            static ISA::MroutineTableEntry buf[ISA::MroutineTableMaxEntryNum];
            assert(sizeof(buf) >= cacheLineSz);

            getMemRawPtr(pkt, buf, cacheLineSz, traceData);

            if(pkt->isError()) {
                panic("MENTER: memory fetch (%s) failed: %s", pkt->getAddrRange().to_string(), pkt->print());
            }

            const RegVal mbr = xc->readMetalReg(metal_reg::MBR);

            Addr loadAddr;
            unsigned int count;
            this->calcLoadAddr(mbr, tc->getSystemPtr()->cacheLineSize(), this->imm, loadAddr, count);

            isa->loadMroutineTable(buf, count, this->imm + (loadAddr - mbr) / (sizeof(ISA::MroutineTableEntry)));

            return NoFault;
        }

        Fault Menter64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            // not maintaining this because we only need TimingSimpleCPU
            panic("unimplemented.");

            // ISA * isa = static_cast<ISA *>(xc->tcBase()->getIsaPtr());
            // metal_reg::MSR_t msr = xc->readMetalReg(metal_reg::MSR);

            // DPRINTF(Metal, "MENTER: MBR = 0x%lx, mroutine = %d\n", xc->readMetalReg(metal_reg::MBR), this->imm);

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

            // must be in Metal mode to mexit
            if (!metal_reg::isInMetalMode(msr)) {
                return std::make_shared<UndefinedInstruction>(machInst, true, mnemonic);
            }

            DPRINTF(Metal, "MEXIT: MLR = 0x%x, flags = 0x%x\n", ret, this->imm);

            // set new PC
            const Addr target_addr = purifyTaggedAddr(ret, xc->tcBase(), currEL(xc->tcBase()), true);
            PCState pcState;
            set(pcState, xc->pcState());
            pcState.instNPC(target_addr);
            xc->pcState(pcState);

            // handle instruction skip and intercept mask flags
            if (this->imm & 0b01) {
                msr.im = 1;
            }
            if (this->imm & 0b10) {
                msr.is = 1;
            }
            if (this->imm != 0) {
                xc->setMetalReg(metal_reg::MSR, msr);
            }

            // decrease Metal level
            msr = xc->readMetalReg(metal_reg::MSR);
            msr.lv = msr.lv - 1;
            xc->setMetalReg(metal_reg::MSR, msr);

            return NoFault;
        }

        // wmr
        Wmr64::Wmr64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg) : MetalRegOp("wmr", _machInst, IntAluOp, _mreg, _greg)
        {
            setSrcRegIdx(_numSrcRegs++, gem5::ArmISA::couldBeZero(gReg) ? RegId() : intRegClass[gReg]);
            setDestRegIdx(_numDestRegs++, metalRegClass[mReg]);
            _numTypedDestRegs[metalRegClass.type()]++;

            this->flags[IsInteger] = true;
            this->flags[IsLoad] = metal_reg::isMetalRegWriteMemAccess(_mreg);
        }

        Fault Wmr64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            ThreadContext *tc = xc->tcBase();
            ISA * isa = static_cast<ISA *>(tc->getIsaPtr());
            metal_reg::MSR_t msr = xc->readMetalReg(metal_reg::MSR);

            DPRINTF(Metal, "WMR: mReg = %d, gReg = %d\n", mReg, gReg);

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
            RegVal v = xc->getRegOperand(this, 0);
            switch(this->mReg) {
                case metal_reg::MIB: {
                    ISA::InstInterceptTable * p = reinterpret_cast<ISA::InstInterceptTable*>(isa->allocMetalContext(sizeof(ISA::InstInterceptTable)));
                    static const std::vector<bool> byte_enable(sizeof(ISA::InstInterceptTable), true);
                    fault = readMemAtomic(xc, static_cast<Addr>(v),
                                            reinterpret_cast<uint8_t*>(p),
                                            sizeof(ISA::InstInterceptTable),
                                            ArmISA::MMU::AllowUnaligned,
                                            byte_enable);
                    break;
                }
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
            } else {
                isa->freeMetalContext();
            }

            return fault;
        }

        Fault Wmr64::initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const
        {
            metal_reg::MSR_t msr = xc->readMetalReg(metal_reg::MSR);

            DPRINTF(Metal, "WMR: mReg = %d, gReg = %d\n", mReg, gReg);

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
            RegVal v = xc->getRegOperand(this, 0);
            switch(this->mReg) {
                case metal_reg::MIB: {
                    static ISA::InstInterceptTable p;
                    fault = initiateMemRead(xc, traceData, v, p, ArmISA::MMU::AllowUnaligned);
                    break;
                }
                default: {
                    panic("shouldn't get here!\n");
                }
            }

            return fault;
        }

        Fault Wmr64::completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const
        {
            ThreadContext *tc = xc->tcBase();
            ISA * isa = static_cast<ISA *>(tc->getIsaPtr());

            switch(this->mReg) {
                case metal_reg::MIB: {
                    ISA::InstInterceptTable * p = reinterpret_cast<ISA::InstInterceptTable*>(isa->allocMetalContext(sizeof(ISA::InstInterceptTable)));
                    getMemRaw(pkt, *p, traceData);
                    break;
                }
                default: {
                    panic("Setting Metal reg %d does not require memory access.\n", mReg);
                }
            }

            xc->setMetalReg(mReg, xc->getRegOperand(this, 0));

            return NoFault;
        }

        // rmr
        Rmr64::Rmr64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg) : MetalRegOp("rmr", _machInst, IntAluOp, _mreg, _greg)
        {
            setDestRegIdx(_numDestRegs++, gem5::ArmISA::couldBeZero(gReg) ? RegId() : intRegClass[gReg]);
            setSrcRegIdx(_numSrcRegs++, metalRegClass[mReg]);
            _numTypedDestRegs[intRegClass.type()]++;

            this->flags[IsInteger] = true;
        }

        Fault Rmr64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            metal_reg::MSR_t msr = xc->readMetalReg(metal_reg::MSR);

            DPRINTF(Metal, "RMR: mReg = %d, gReg = %d\n", mReg, gReg);

            if (!metal_reg::canReadMetalReg(msr, mReg))
            {
                return std::make_shared<UndefinedInstruction>(machInst, true, mnemonic);
            }

            RegVal v = xc->readMetalReg(mReg);
            xc->setRegOperand(this, 0, v);

            return NoFault;
        }

        // rar64
        Rar64::Rar64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg) : MetalRegOp("rar", _machInst, IntAluOp, _mreg, _greg)
        {
            setSrcRegIdx(_numSrcRegs++, gem5::ArmISA::couldBeZero(gReg) ? RegId() : intRegClass[gReg]);
            setDestRegIdx(_numDestRegs++, metalRegClass[mReg]);
            _numTypedDestRegs[metalRegClass.type()]++;

            this->flags[IsInteger] = true;
        }

        Fault Rar64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            metal_reg::MSR_t msr = xc->readMetalReg(metal_reg::MSR);
            RegVal idx = xc->getRegOperand(this, 0);

            DPRINTF(Metal, "RAR: mReg = %d, gReg = %d(0x%lx)\n", mReg, idx);

            if (!metal_reg::canWriteMetalReg(msr, mReg))
            {
                return std::make_shared<UndefinedInstruction>(machInst, true, mnemonic);
            }

            const RegId &id = intRegClass[idx];
            RegVal v = xc->getReg(id);
            xc->setMetalReg(mReg, v);

            return NoFault;
        }

        // war64
        War64::War64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg) : MetalRegOp("war", _machInst, IntAluOp, _mreg, _greg)
        {
            setSrcRegIdx(_numSrcRegs++, metalRegClass[mReg]);
            setSrcRegIdx(_numDestRegs++, gem5::ArmISA::couldBeZero(gReg) ? RegId() : intRegClass[gReg]);
            setDestRegIdx(_numDestRegs++, gem5::ArmISA::couldBeZero(gReg) ? RegId() : intRegClass[gReg]);
            _numTypedDestRegs[intRegClass.type()]++;

            this->flags[IsInteger] = true;
        }

        Fault War64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            metal_reg::MSR_t msr = xc->readMetalReg(metal_reg::MSR);
            RegVal idx = xc->getRegOperand(this, 1);
            DPRINTF(Metal, "WAR: mReg = %d, gReg = %d(0x%lx)\n", mReg, gReg, idx);

            if (!metal_reg::canReadMetalReg(msr, mReg))
            {
                return std::make_shared<UndefinedInstruction>(machInst, true, mnemonic);
            }

            RegVal v = xc->readMetalReg(mReg);
            const RegId & reg = intRegClass[idx];
            xc->setReg(reg, v);

            return NoFault;
        }

        // rtlb
        Rtlb64::Rtlb64(ExtMachInst _machInst, RegIndex _rl, RegIndex _rm,
                       RegIndex _rn)
                       : MetalThreeRegOp("rtlb", _machInst, IntAluOp, _rl, _rm,
                                         _rn)
        {
            this->flags[IsInteger] = true;
        }

        Fault Rtlb64::execute(ExecContext *xc, trace::InstRecord *traceData)
            const
        {
            metal_reg::MSR_t msr = xc->readMetalReg(metal_reg::MSR);

            DPRINTF(Metal, "RTLB: rl = %d, rm = %d, rn = %d\n", rl, rm, rn);

            if (!metal_reg::canWriteMetalReg(msr, rl)
                || !metal_reg::canWriteMetalReg(msr, rm)
                || !metal_reg::canReadMetalReg(msr, rn))
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
                    xc->tcBase()->getMMUPtr()->itb)
                    ->getEntry(va);

            // Create and fill a new page table entry

            tei.set_isHyp(te->isHyp);
            tei.set_asid(te->asid);
            tei.set_vmid(te->vmid);
            // insertBits(ld.data, )
            switch (te->N) {
                // type == Page
                case Grain4KB:
                case Grain16KB:
                case Grain64KB:
                    // ld.type() set for block
                    insertBits(ld.data, 1, 0, 0x1);
                    ld.grainSize = (GrainSize)te->N;
                    break;

                // type == Block
                case 21:    // 2 MiB
                case 30:    // 1 GiB
                    // ld.type() set for page/table
                    insertBits(ld.data, 1, 0, 0x3);
                    ld.grainSize = Grain4KB;
                    break;
                case 25:    // 32 MiB
                    // ld.type() set for page/table
                    insertBits(ld.data, 1, 0, 0x3);
                    ld.grainSize = Grain16KB;
                    break;
                case 29:    // 256 MiB
                case 42:    // 4 TiB
                    // ld.type() set for page/table
                    insertBits(ld.data, 1, 0, 0x3);
                    ld.grainSize = Grain64KB;
                    break;
                default:
                    // panix - bad descriptor?
                    break;
            }

            // assert(ld.offsetBits() == te->N);

            va              = te->vpn << te->N;
            // pfn
            insertBits(ld.data, 47, te->N, mbits(te->pfn, 47, te->N));
            insertBits(ld.data, 15, 12, bits(te->pfn, 51, 48));
            // domain always TlbEntry::DomainType::Client for LongDescriptor
            // te.domain         = ld.domain();
            ld.lookupLevel  = te->lookupLevel;
            insertBits(ld.data, 5, te->ns);
            tei.set_isSecure(!te->nstid);
            // xn
            insertBits(ld.data, 54, te->xn);
            tei.set_type(te->type == TypeTLB::instruction ? true : false);
            tei.set_el(te->el);
            // ld.global()
            insertBits(ld.data, 11, !te->global);
            // ld.pxn()
            insertBits(ld.data, 53, te->pxn);
            // ld.ap()
            insertBits(ld.data, 7, 6, te->ap);
            tei.set_mtype(te->mtype);
            tei.set_nc(te->nonCacheable);
            // Attributes formatted according to the 64-bit PAR
            tei.set_attr(te->attributes >> 56);
            // ld.sh()
            insertBits(ld.data, 9, 8, (te->attributes >> 7) & 0b11);


            xc->setMetalReg(rl, (RegVal)ld.data);
            xc->setMetalReg(rm, (RegVal)tei.data);

            return NoFault;
        }

        // wtlb
        Wtlb64::Wtlb64(ExtMachInst _machInst, RegIndex _rl, RegIndex _rm,
                       RegIndex _rn) : MetalThreeRegOp("wtlb", _machInst,
                                                       IntAluOp, _rl, _rm, _rn)
        {
            this->flags[IsInteger] = true;
        }

        Fault Wtlb64::execute(ExecContext *xc, trace::InstRecord *traceData)
            const
        {
            metal_reg::MSR_t msr = xc->readMetalReg(metal_reg::MSR);

            DPRINTF(Metal, "WTLB: rl = %d, rm = %d, rn = %d\n", rl, rm, rn);

            if (!metal_reg::canReadMetalReg(msr, rl)
                || !metal_reg::canReadMetalReg(msr, rm)
                || !metal_reg::canReadMetalReg(msr, rn))
            {
                return std::make_shared<UndefinedInstruction>(machInst, false, mnemonic);
            }

            RegVal desc = xc->readMetalReg(rl);
            RegVal info = xc->readMetalReg(rm);
            RegVal vaddr = xc->readMetalReg(rn);

            TableWalker::LongDescriptor ld;
            ld.data = desc;
            ld.aarch64 = true;

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

            dynamic_cast<ArmISA::TLB *>(xc->tcBase()->getMMUPtr()->itb)
                ->insert(te);

            return NoFault;
        }

    } // namespace ArmISA
} // namespace gem5
