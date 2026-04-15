#pragma once

#include "arch/arm/insts/metal/common.hh"
#include "arch/arm/pagetable.hh"
#include "arch/arm/tlbi_op.hh"

namespace gem5 {
    namespace ArmISA {
    namespace metal { namespace inst {
        BitUnion64(FTLBAttr)
            Bitfield<1, 0> el; // el = 0-3
            Bitfield<2> nstid; // True if the entry was brought in from a non-secure page table
            Bitfield<3> match_va;
            Bitfield<4> match_asid;
            Bitfield<5> match_vmid;
            Bitfield<6> broadcast;
            Bitfield<47, 32> asid; // 16 bit asid
            Bitfield<63, 48> vmid; // 16 bit vmid
        EndBitUnion(FTLBAttr)

        class FTLBOp : public TLBIOp
        {
        private:
            unsigned int vmid;
            bool match_vmid;

            unsigned int asid;
            bool match_asid;

            Addr va;
            bool match_va;

        public:
            FTLBOp(ExceptionLevel _el, bool _secure,
                unsigned int _vmid,
                bool _match_vmid,
                unsigned int _asid,
                bool _match_asid,
                Addr _va,
                bool _match_va);

            void operator()(ThreadContext* tc) override;

            std::string print() const override;

            bool match(TlbEntry *entry, vmid_t curr_vmid) const override;

            bool stage1Flush() const override;

            bool stage2Flush() const override;

            FTLBOp makeStage2() const;
        };

        class Ftlb64 : public MetalReg2Op
        {
        public:
            Ftlb64(ExtMachInst _machInst, RegIndex _r1, RegIndex _r2);

            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
        };
    }}
    }
}
