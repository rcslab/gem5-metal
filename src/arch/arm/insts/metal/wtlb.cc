#include "arch/arm/insts/metal/wtlb.hh"
#include "arch/arm/table_walker.hh"
#include <array>

namespace gem5 {
    namespace ArmISA {
    namespace metal { namespace inst {
        static constexpr unsigned int WTLB_MAX_PGSHIFT = 44;
        static constexpr unsigned int WTLB_MIN_PGSHIFT = 12;
        enum VSpecType {
            Data = 0,
            Inst = 1,
            Unified = 2,
            Reserved = 3
        };
        BitUnion64(TLBVSpec)
            Bitfield<63, WTLB_MIN_PGSHIFT> vaddr;
            Bitfield<10, 7> mapid; // Metal access permission ID
            Bitfield<6> map; // Metal access permissions 
            Bitfield<5, 1> sz; // real page size bits = (12 + 2^5) - 2^5 (16TB to 4KB)
            Bitfield<0> itlb;
        EndBitUnion(TLBVSpec)

        BitUnion64(TLBPSpec)
            Bitfield<63, WTLB_MIN_PGSHIFT> paddr;
        EndBitUnion(TLBPSpec)
        
        BitUnion64(TLBExtAttr)
            Bitfield<1, 0> el; // el = 0-3
            Bitfield<3, 2> ap; // access permission
            Bitfield<4> xn; // whether the page is not executable
            Bitfield<5> pxn; // whether the page is not executable by privileged
            Bitfield<6> hyp; // whether this tlb is stage 2
            Bitfield<7> ns; // True if the entry targets the non-secure physical address space
            Bitfield<8> nstid; // True if the entry was brought in from a non-secure page table
            Bitfield<9> ng; // whether the page is not global 
            Bitfield<11, 10> sh; // shareability
            Bitfield<19, 12> mair; // mair bits
            Bitfield<35, 20> asid; // 16 bit asid
            Bitfield<51, 36> vmid; // 16 bit vmid
        EndBitUnion(TLBExtAttr)

        static inline std::string printTlbAttr(const TLBVSpec vspec, const TLBPSpec pspec, const TLBExtAttr attr)
        {
            return csprintf("[itlb: %u, vaddr: %#llx, paddr: %#llx, sz: %u, map: %u, mapid: %u] + "
                    "[el: %u, ap: %u, xn: %u, pxn: %u, mair: %#x, sh: %u, ng: %u, "
                    "hyp: %u, asid: %u, vmid: %u, ns: %u, nstid: %u]",
                    vspec.itlb, vspec.vaddr << WTLB_MIN_PGSHIFT, pspec.paddr << WTLB_MIN_PGSHIFT, vspec.sz, vspec.map, vspec.mapid,
                    attr.el, attr.ap, attr.xn, attr.pxn, attr.mair, attr.sh, attr.ng, 
                    attr.hyp, attr.asid, attr.vmid, attr.ns, attr.nstid);
        }

        Wtlb64::Wtlb64(ExtMachInst _machInst, RegIndex _r1, RegIndex _r2,
                       RegIndex _r3) : MetalReg3Op("wtlb", _machInst,
                                                       IntAluOp, _r1, _r2, _r3)
        {
            setSrcRegIdx(_numSrcRegs++, intRegClass[r1]);
            setSrcRegIdx(_numSrcRegs++, intRegClass[r2]);
            setSrcRegIdx(_numSrcRegs++, intRegClass[r3]);

            this->flags[IsNonSpeculative] = true;
            this->flags[IsSerializeAfter] = true;
        }

        Fault Wtlb64::execute(ExecContext *xc, trace::InstRecord *traceData)
            const
        {
            const auto &mist = xc->getMetalState();

            if (mist.getLevel() == 0)
            {
                METAL_DBGPRINT(INSTS, WTLB, "Permission denied: MetalState = [%s].\n", mist.toStr().c_str());
                return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }

            const auto vspec = static_cast<TLBVSpec>(xc->getRegOperand(this, 0));
            const auto pspec = static_cast<TLBPSpec>(xc->getRegOperand(this, 1));
            const auto eattrs = static_cast<TLBExtAttr>(xc->getRegOperand(this, 2));

            METAL_DBGPRINT(INSTS, WTLB, "WTLB: %s.\n", printTlbAttr(vspec, pspec, eattrs));

            TlbEntry te;

            // fixed attributes
            te.valid = true;
            te.longDescFormat = true;
            te.partial = false;

            // attributes are split into 2 parts
            // ARM page table descriptor (long format) bits
            // and extraAttr that come from the third Reg

            // long descriptor bits
            te.type = vspec.itlb ? TypeTLB::instruction : TypeTLB::data;
            te.lookupLevel = enums::ArmLookupLevel::L3;
            te.tg = ReservedGrain;
            te.N = WTLB_MAX_PGSHIFT - vspec.sz;
            const Addr pvaddr = purifyTaggedAddr(vspec.vaddr << WTLB_MIN_PGSHIFT, 
                            xc->tcBase(), currEL(xc->tcBase()), 
                        te.type == TypeTLB::unified || te.type == TypeTLB::instruction);
            te.vpn = pvaddr >> te.N;
            te.size = (1ull << te.N) - 1;
            te.pfn = (pspec.paddr << WTLB_MIN_PGSHIFT) >> te.N;
            te.map = vspec.map;
            te.mapid = vspec.mapid;

            te.domain = TlbEntry::DomainType::Client; // Domain will be deprecated so hardcore to client
            te.xn = eattrs.xn;
            te.pxn = eattrs.pxn;
            te.ap = eattrs.ap;
            te.hap = eattrs.ap;
            te.global = !eattrs.ng;

            // extra attributes
            te.el = static_cast<ExceptionLevel>(static_cast<int>(eattrs.el));
            te.asid = eattrs.asid;
            te.isHyp = eattrs.hyp;
            te.vmid = eattrs.vmid;
            // METAL_XXX: wtf do these fields mean?
            te.nstid = eattrs.nstid;
            te.ns = eattrs.ns;

            // set memory type and cacheability/shareability
            if (eattrs.hyp) {
                // TableWalker::memAttrsAArch64Stage2(te, ld.memAttr());
                METAL_DBGPRINT(INSTS, WTLB, "Does not support stage 2 TLB entries!.\n");
                return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            } else {
                TableWalker::memAttrsAArch64Stage1(te, eattrs.sh, eattrs.mair);
            }

            MMU * mmu = dynamic_cast<MMU *>(xc->tcBase()->getMMUPtr());
            assert(mmu);

            TLB * tlb = mmu->getTlb(vspec.itlb ? BaseMMU::Execute : BaseMMU::Read, te.isHyp);
            tlb->multiInsert(te);

            if (traceData) {
                std::array<RegVal, 3> vals{vspec, pspec, eattrs};
                traceData->setData(vals);
            }

            return NoFault;
        }
    }}
    }
}
