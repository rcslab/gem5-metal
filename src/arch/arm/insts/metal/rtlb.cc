#include "arch/arm/insts/metal/rtlb.hh"
#include "arch/arm/insts/metal/wtlb.hh"
#include "arch/arm/pagetable.hh"

namespace gem5 {
    namespace ArmISA {
    namespace metal { namespace inst {
        Rtlb64::Rtlb64(ExtMachInst _machInst, RegIndex _rl, RegIndex _rm,
                       RegIndex _rn)
                       : MetalReg3Op("rtlb", _machInst, IntAluOp, _rl, _rm,
                                         _rn)
        {
            setSrcRegIdx(_numSrcRegs++, intRegClass[r1]);
            setSrcRegIdx(_numSrcRegs++, intRegClass[r3]);
            
            setDestRegIdx(_numDestRegs++,intRegClass[r1]);
            setDestRegIdx(_numDestRegs++,intRegClass[r2]);
            setDestRegIdx(_numDestRegs++,intRegClass[r3]);
            _numTypedDestRegs[intRegClass.type()] += 3;

            this->flags[IsInteger] = true;
        }

        Fault Rtlb64::execute(ExecContext *xc, trace::InstRecord *traceData)
            const
        {
            const auto &mist = xc->getMetalState();

            if (mist.getLevel() == 0)
            {
                METAL_DBGPRINT(INSTS, WTLB, "Permission denied: MetalState = [%s].\n", mist.toStr().c_str());
                return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }

            const TLBVSpec vspec = xc->getRegOperand(this, 0);
            const TLBExtAttr attrs = xc->getRegOperand(this, 1);
            MMU * mmu = dynamic_cast<ArmISA::MMU *>(xc->tcBase()->getMMUPtr());
            assert(mmu);

            const Addr vpn = (vspec.vaddr <<  WTLB_MIN_PGSHIFT) >> (WTLB_MAX_PGSHIFT - vspec.sz);

            const TlbEntry * ent = mmu->lookup(vpn, attrs.asid, attrs.vmid, attrs.hyp, 
                !attrs.ns, true, false, 
                static_cast<ExceptionLevel>(static_cast<int>(attrs.el)), 
                false, 
                false,
                vspec.itlb ? BaseMMU::Execute : BaseMMU::Read);

            TLBVSpec rvspec;
            TLBExtAttr rattrs;
            TLBPSpec rpspec;
            if (ent == nullptr) {
                rvspec = 0;
                rattrs = 0;
                rpspec = 0;
                METAL_DBGPRINT(INSTS, WTLB, "RTLB: %s -> MISS.\n",
                    printTlbAttr(vspec, 0, attrs));
            } else {
                rvspec.itlb = ent->type & TypeTLB::instruction;
                rvspec.map = ent->map;
                rvspec.mapid = ent->mapid;
                rvspec.sz = WTLB_MAX_PGSHIFT - ent->N;
                rvspec.vaddr = (ent->vpn << ent->N) >> WTLB_MIN_PGSHIFT ;
                rpspec.paddr = ent->pAddr(0);
                rpspec.valid = ent->valid;
                rattrs.ap = ent->ap;
                rattrs.asid = ent->asid;
                rattrs.el = ent->el;
                rattrs.hyp = ent->isHyp;
                rattrs.mair = ((ent->attributes) >> 56) & 0b11111111;
                rattrs.ng = !ent->global;
                rattrs.ns = ent->ns;
                rattrs.nstid = ent->nstid;
                rattrs.pxn = ent->pxn;
                rattrs.sh = ((ent->attributes) >> 7) & 0b11;
                rattrs.vmid = ent->vmid;
                rattrs.xn = ent->xn;
                METAL_DBGPRINT(INSTS, WTLB, "RTLB: %s -> %s.\n", 
                    printTlbAttr(vspec, 0, attrs), 
                    printTlbAttr(rvspec, rpspec, rattrs));
            }

            xc->setRegOperand(this, 0, rvspec);
            xc->setRegOperand(this, 1, rpspec);
            xc->setRegOperand(this, 2, rattrs);

            return NoFault;
        }
    }}
    }
}
