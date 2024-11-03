#pragma once

#include "arch/arm/insts/metal/common.hh"

namespace gem5 {
    namespace ArmISA {
    // menter
        class Menter64 : public MetalImmOp8
        {
        public:
            Menter64(ExtMachInst _machInst, uint8_t _imm);

            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
            Fault initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const override;
            Fault completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const override;
            static void doMenter(ThreadContext *xc, Addr npc, Addr lpc);
            static void doMenter(ThreadContext *xc, Addr npc, const ArmStaticInst &inst);
        private:
            static void calcLoadAddr(Addr base, unsigned long align, unsigned int idx, Addr & _loadAddr, unsigned int & _count);
        };
    }
}