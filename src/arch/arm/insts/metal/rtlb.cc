#include "arch/arm/insts/metal/rtlb.hh"

namespace gem5 {
    namespace ArmISA {
    namespace metal { namespace inst {
        Rtlb64::Rtlb64(ExtMachInst _machInst, RegIndex _rl, RegIndex _rm,
                       RegIndex _rn)
                       : MetalReg3Op("rtlb", _machInst, IntAluOp, _rl, _rm,
                                         _rn)
        {
            this->flags[IsInteger] = true;
        }

        Fault Rtlb64::execute(ExecContext *xc, trace::InstRecord *traceData)
            const
        {
            // this needs more thinking but we don't need this feature for now
            panic("unimplemented");

            // metal_reg::MSR_t msr = xc->readMetalReg(metal_reg::MSR);
            // RegVal vaReg = xc->readMetalReg(rn);

            // METAL_DBGPRINT(INSTS, RTLB, "RTLB: vaReg = %s, rm = %s, rn = %s (%#lx).\n",
            //                                         printMetalReg(rl),
            //                                         printMetalReg(rm),
            //                                         printMetalReg(rn), vaReg);

            // if (!metal_reg::canWriteMetalReg(msr, rl)
            //     || !metal_reg::canWriteMetalReg(msr, rm)
            //     || !metal_reg::canReadMetalReg(msr, rn)
            //     || (!metal_reg::isPrivilegeCheckDisabled(msr) && !metal_reg::isInMetalMode(msr)))
            // {
            //     return std::make_shared<UndefinedInstruction>(machInst, false, mnemonic);
            // }

            // TableWalker::LongDescriptor ld;
            // ld.aarch64 = true;

            // TlbEntry::Lookup lookup;
            // lookup.va = vaReg;
            // lookup.ignoreAsn = true;
            // lookup.targetEL = currEL(xc->tcBase());

            // ArmISA::MMU * mmu = dynamic_cast<ArmISA::MMU *>(xc->tcBase()->getMMUPtr());
            // assert(mmu);

            // vaReg = purifyTaggedAddr(vaReg, xc->tcBase(), currEL(xc->tcBase()), false);

            // // read dtb by default
            // // METAL_XXX: support more flags
            // auto te = mmu->lookup(vaReg, 0, 0, false, false, false, true, currEL(xc->tcBase()), false, false, BaseMMU::Mode::Read);
            // if (!te) {
            //     panic("tlb entry does not exist.\n");
            // }

            // // Create and fill a new page table entry
            // tei.set_isHyp(te->isHyp);
            // tei.set_asid(te->asid);
            // tei.set_vmid(te->vmid);
            // // ld.data = insertBits(ld.data, )
            // switch (te->N) {
            //     // type == Page
            //     case Grain4KB:
            //     case Grain16KB:
            //     case Grain64KB:
            //         ld.data = insertBits(ld.data, 1, 0, 0x3);
            //         ld.grainSize = (GrainSize)te->N;
            //         break;

            //     // type == Block
            //     case 21:    // 2 MiB
            //     case 30:    // 1 GiB
            //         ld.data = insertBits(ld.data, 1, 0, 0x1);
            //         ld.grainSize = Grain4KB;
            //         break;
            //     case 25:    // 32 MiB
            //         ld.data = insertBits(ld.data, 1, 0, 0x1);
            //         ld.grainSize = Grain16KB;
            //         break;
            //     case 29:    // 256 MiB
            //     case 42:    // 4 TiB
            //         ld.data = insertBits(ld.data, 1, 0, 0x1);
            //         ld.grainSize = Grain64KB;
            //         break;
            //     default:
            //         panic("Unknown page size: %d.", te->N);
            //         break;
            // }

            // // pfn
            // ld.data = insertBits(ld.data, 47, te->N, bits(te->pfn << te->N, 47,
            //                      te->N));
            // if (te->N == 16)
            //     ld.data = insertBits(ld.data, 15, 12, bits(te->pfn << te->N,
            //                          51, 48)); // 64k pages
            // // domain always TlbEntry::DomainType::Client for LongDescriptor
            // // te.domain         = ld.domain();
            // ld.lookupLevel  = te->lookupLevel;
            // ld.data = insertBits(ld.data, 5, te->ns);
            // tei.set_isSecure(!te->nstid);
            // // xn
            // ld.data = insertBits(ld.data, 54, te->xn);
            // tei.set_type(te->type == TypeTLB::instruction ? true : false);
            // tei.set_el(te->el);
            // // ld.global()
            // ld.data = insertBits(ld.data, 11, !te->global);
            // // ld.pxn()
            // ld.data = insertBits(ld.data, 53, te->pxn);
            // // ld.ap()
            // ld.data = insertBits(ld.data, 7, 6, te->ap);
            // tei.set_mtype(te->mtype);
            // tei.set_nc(te->nonCacheable);
            // // Attributes formatted according to the 64-bit PAR
            // tei.set_attr(te->attributes >> 56);
            // // ld.sh()
            // ld.data = insertBits(ld.data, 9, 8, (te->attributes >> 7) & 0b11);

            // tei.set_ao(te->ao);
            // tei.set_ai(te->aoid);

            // xc->setMetalReg(rl, (RegVal)ld.data);
            // xc->setMetalReg(rm, (RegVal)tei.data);
            //return NoFault;
        }
    }}
    }
}
