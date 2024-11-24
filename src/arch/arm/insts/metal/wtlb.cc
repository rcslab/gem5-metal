#include "arch/arm/insts/metal/wtlb.hh"
#include "arch/arm/table_walker.hh"

namespace gem5 {
    namespace ArmISA {
        BitUnion64(TlbExtAttr)
            Bitfield<0> itlb; // 1 = inst tlb, 0 = data tlb
            Bitfield<2, 1> el; // el = 0-3
            Bitfield<4, 3> translv; // 0 - 3
            Bitfield<6, 5> pgsz; // 0: 4k 1: 16k 2: 64k
            Bitfield<22, 7> asid; // 8 bit asid
            Bitfield<23> hyp; // whether this is for hypervisor
            Bitfield<39, 24> vmid; // 16 bit vmid
            Bitfield<40> ao; // access override
            Bitfield<44, 41> aoid; // access override index
            Bitfield<52, 45> mair; // mair fields for stage 1
             // True if the entry targets the non-secure physical address space
            Bitfield<53> ns;
            // True if the entry was brought in from a non-secure page table
            Bitfield<54> nstid;
        EndBitUnion(TlbExtAttr)

        static inline std::string printTlbExtAttr(const TlbExtAttr attr)
        {
            return csprintf("itlb: %d, el: %d, translv: %d, pgsz: %d, asid: %d, hyp: %d, vmid: %d, ao: %d, aoid: %d, mair: %#x, ns: %#x, nstid: %#x",
                    attr.itlb, attr.el, attr.translv,
                    attr.pgsz, attr.asid, attr.hyp,
                    attr.vmid, attr.ao, attr.aoid,
                    attr.mair, attr.ns, attr.nstid);
        }

        static inline GrainSize tlbExtAttrToGrainSize(const TlbExtAttr attr)
        {
            static std::array<GrainSize, 4> lookup {
                Grain4KB,
                Grain16KB,
                Grain64KB,
                ReservedGrain
            };

            return lookup.at(attr.pgsz);
        }

        Wtlb64::Wtlb64(ExtMachInst _machInst, RegIndex _rl, RegIndex _rm,
                       RegIndex _rn) : MetalRegOp3("wtlb", _machInst,
                                                       IntAluOp, _rl, _rm, _rn)
        {
            this->flags[IsInteger] = true;
        }

        Fault Wtlb64::execute(ExecContext *xc, trace::InstRecord *traceData)
            const
        {
            ThreadContext *tc = xc->tcBase();
            metal_reg::MSR_t msr = tc->readMetalMiscRegNoEffect(metal_reg::MSR);
            RegVal desc = tc->readMetalReg(rl);
            RegVal info = tc->readMetalReg(rm);
            RegVal vaddr = tc->readMetalReg(rn);

            METAL_DBGPRINT(INSTS, WTLB, "WTLB: descReg = %s (%#lx), extReg = %s (%#lx), vaReg = %s (%#lx). ExtAttrs = %s\n",
                                            printMetalReg(rl), desc,
                                            printMetalReg(rm), info,
                                            printMetalReg(rn), vaddr,
                                            printTlbExtAttr(info));

            if (!metal_reg::isInMetalMode(msr) || !metal_reg::isPrivInstsEnabled(msr))
            {
                METAL_DBGPRINT(INSTS, WTLB, "Permission denied: MSR = 0x%lx.\n", msr);
                return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }

            const auto ea = static_cast<TlbExtAttr>(info);
            TlbEntry te;
            TableWalker::LongDescriptor ld;

            ld.data = desc;
            ld.aarch64 = true;
            // lookup level is used with granule to collectively determine the size of the entry
            ld.lookupLevel = static_cast<TlbEntry::LookupLevel>(static_cast<int>(ea.translv));
            ld.grainSize = tlbExtAttrToGrainSize(ea);

            if (ld.grainSize == ReservedGrain ||
                (ld.type() != TableWalker::LongDescriptor::EntryType::Block &&
                ld.type() != TableWalker::LongDescriptor::EntryType::Page)) {
                // must have a supported page size and be a block/page descriptor
                METAL_DBGPRINT(INSTS, WTLB, "Invalid TLB attribute.\n");
                return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }

            // fixed attributes
            te.valid = true;
            te.longDescFormat = true;
            te.partial = false;

            // attributes are split into 2 parts
            // ARM page table descriptor (long format) bits
            // and extraAttr that come from the third Reg

            // long descriptor bits
            te.lookupLevel = ld.lookupLevel;
            te.N = ld.offsetBits(); // offsetBits only make sense after setting grainSize and lookupLevel
            te.vpn = vaddr >> te.N;
            te.size = (1 << te.N) - 1;
            te.pfn = ld.pfn();
            te.domain = ld.domain();
            te.xn = ld.xn();
            te.pxn = ld.pxn();
            te.ap = ld.ap();
            te.hap = ld.ap();
            te.global = !ld.ng();

            // extra attributes
            te.el = static_cast<ExceptionLevel>(static_cast<int>(ea.el));
            te.asid = ea.asid;
            te.isHyp = ea.hyp;
            te.vmid = ea.vmid;
            te.type = ea.itlb ? TypeTLB::instruction : TypeTLB::data;
            te.ao = ea.ao;
            te.aoid = ea.aoid;
            // METAL_XXX: wtf do these fields mean?
            te.nstid = ea.nstid;
            te.ns = ea.ns;

            // set memory type and cacheability/shareability
            if (ea.hyp) {
                TableWalker::memAttrsAArch64Stage2(te, ld.memAttr());
            } else {
                TableWalker::memAttrsAArch64Stage1(te, ld.sh(), ea.mair);
            }

            MMU * mmu = dynamic_cast<MMU *>(xc->tcBase()->getMMUPtr());
            assert(mmu);

            TLB * tlb = mmu->getTlb(ea.itlb ? BaseMMU::Execute : BaseMMU::Read, te.isHyp);
            tlb->insert(te);

            return NoFault;
        }

        std::string Wtlb64::generateDisassembly(
                Addr pc, const loader::SymbolTable *symtab) const
        {
            std::stringstream ss;
            ss << MetalDisasmPrefix;
            printMnemonic(ss, "", false);
            printMetalReg(ss, rl);
            ccprintf(ss, ", ");
            printMetalReg(ss, rm);
            ccprintf(ss, ", ");
            printMetalReg(ss, rn);
            return ss.str();
        }
    }
}