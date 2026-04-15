#pragma once

#include "arch/arm/insts/metal/common.hh"

namespace gem5 {
    namespace ArmISA {
    namespace metal { namespace inst {
        static constexpr unsigned int WTLB_MIN_PGSHIFT = 12;
        static constexpr unsigned int WTLB_MAX_PGSHIFT = WTLB_MIN_PGSHIFT + 32;
        BitUnion64(TLBVSpec)
            Bitfield<63, WTLB_MIN_PGSHIFT> vpn;
            Bitfield<10, 7> mapid; // Metal access permission ID
            Bitfield<6> map; // Metal access permissions
            Bitfield<5, 1> sz; // real page size bits = (12 + 2^5) - 2^5 (16TB to 4KB)
            Bitfield<0> itlb;
        EndBitUnion(TLBVSpec)

        BitUnion64(TLBPSpec)
            Bitfield<63, WTLB_MIN_PGSHIFT> ppn;
            Bitfield<0> valid; // used by RTLB -- whether the result is valid
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
            const unsigned int N = WTLB_MIN_PGSHIFT + vspec.sz;
            return csprintf("[itlb: %u, vaddr: %#llx, paddr: %#llx, N: %u, map: %u, mapid: %u] + "
                    "[el: %u, ap: %u, xn: %u, pxn: %u, mair: %#x, sh: %u, ng: %u, "
                    "hyp: %u, asid: %u, vmid: %u, ns: %u, nstid: %u]",
                    vspec.itlb, vspec.vpn << N, pspec.ppn << N, N, vspec.map, vspec.mapid,
                    attr.el, attr.ap, attr.xn, attr.pxn, attr.mair, attr.sh, attr.ng,
                    attr.hyp, attr.asid, attr.vmid, attr.ns, attr.nstid);
        }

        class Wtlb64 : public MetalReg3Op
        {
        public:
            Wtlb64(ExtMachInst _machInst, RegIndex _rl, RegIndex _rm,
                   RegIndex _rn);

            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
        };
    }}
    }
}
