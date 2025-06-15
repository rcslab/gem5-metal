#pragma once

#include "arch/arm/insts/metal/common.hh"

namespace gem5 {
    namespace ArmISA {
    // menter
        class Menter64 : public MetalImmOp
        {
        public:
            Menter64(ExtMachInst _machInst, uint _imm);

            virtual Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
            virtual Fault preExec(ExecContext *xc, trace::InstRecord *traceData) override;
            // Fault initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const override;
            // Fault completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const override;
            static void doMenter(ExecContext *xc, const StaticInst * inst, Addr npc, Addr lpc, int mlr_idx);
        // private:
        //     static void calcLoadAddr(Addr base, unsigned long align, unsigned int idx, Addr & _loadAddr, unsigned int & _count);
        };
    }
}
