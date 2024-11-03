#pragma once

#include "arch/arm/insts/metal/common.hh"

namespace gem5 {
    namespace ArmISA {
        template <typename T>
        class Pldri : public MetalPMemRegImmOp
        {
            static_assert(isPowerOf2(sizeof(T)) && (sizeof(T) <= sizeof(RegVal)) && (sizeof(T) > 0));
        public:
            Pldri(ExtMachInst _machInst, RegIndex _dReg, RegIndex _sReg, int32_t _imm, Mode _mode);
            Fault initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const override;
            Fault completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const override;
            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
        };
        template class Pldri<uint8_t>;
        template class Pldri<uint16_t>;
        template class Pldri<uint32_t>;
        template class Pldri<uint64_t>;


        template <typename T>
        class Pldrr : public MetalPMemRegOp
        {
            static_assert(isPowerOf2(sizeof(T)) && (sizeof(T) <= sizeof(RegVal)) && (sizeof(T) > 0));
        public:
            Pldrr(ExtMachInst _machInst, RegIndex _dReg, RegIndex _bReg, RegIndex _oReg);
            Fault initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const override;
            Fault completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const override;
            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
        };
        template class Pldrr<uint8_t>;
        template class Pldrr<uint16_t>;
        template class Pldrr<uint32_t>;
        template class Pldrr<uint64_t>;
    }
}