#include "arch/arm/insts/metal/wtlb.hh"
#include "arch/arm/table_walker.hh"
#include <array>

namespace gem5 {
    namespace ArmISA {
    namespace metal { namespace inst {
        Wtlb64::Wtlb64(ExtMachInst _machInst, RegIndex _r1, RegIndex _r2,
                       RegIndex _r3) : MetalReg3Op("wtlb", _machInst,
                                                       IntAluOp, _r1, _r2, _r3)
        {
            setSrcRegIdx(_numSrcRegs++, intRegClass[r1]);
            setSrcRegIdx(_numSrcRegs++, intRegClass[r2]);
            setSrcRegIdx(_numSrcRegs++, intRegClass[r3]);

            // this->flags[IsNonSpeculative] = true;
            // this->flags[IsSerializeAfter] = true;
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
                            xc->tcBase(), currEL(xc->tcBase()), vspec.itlb);
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
