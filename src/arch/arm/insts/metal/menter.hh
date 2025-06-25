#pragma once

#include "arch/arm/insts/metal/common.hh"

namespace gem5 { namespace ArmISA {
namespace metal { namespace inst{
    // menter
        class Menter64 : public MetalImmOp
        {
        public:
            Menter64(ExtMachInst _machInst, uint _imm);
            Menter64(const char * mnem, ExtMachInst _machInst, uint _imm);

            Fault execute(ExecContext *xc, trace::InstRecord *traceData, Addr mlr) const;
            virtual Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
            virtual Fault preExec(ExecContext *xc, trace::InstRecord *traceData) override;
            // Fault initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const override;
            // Fault completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const override;
            // static void enterMetalMode(ExecContext *xc, const StaticInst * inst, Addr npc, Addr lpc, int mlr_idx);
        // private:
        //     static void calcLoadAddr(Addr base, unsigned long align, unsigned int idx, Addr & _loadAddr, unsigned int & _count);
        };
}}
}}
