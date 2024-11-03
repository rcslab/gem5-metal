#pragma once

#include "arch/arm/insts/metal/common.hh"

namespace gem5 {
    namespace ArmISA {
        template <typename T>
        class Pstri : public MetalPMemRegImmOp
        {
            static_assert(isPowerOf2(sizeof(T)) && (sizeof(T) <= sizeof(RegVal)) && (sizeof(T) > 0));
        public:
            Pstri(ExtMachInst _machInst, RegIndex _dReg, RegIndex _sReg, int32_t _imm, Mode _mode);
            Fault initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const override;
            Fault completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const override;
            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
        };
        template class Pstri<uint8_t>;
        template class Pstri<uint16_t>;
        template class Pstri<uint32_t>;
        template class Pstri<uint64_t>;

        template <typename T>
        class Pstrr : public MetalPMemRegOp
        {
            static_assert(isPowerOf2(sizeof(T)) && (sizeof(T) <= sizeof(RegVal)) && (sizeof(T) > 0));
        public:
            Pstrr(ExtMachInst _machInst, RegIndex _dReg, RegIndex _bReg, RegIndex _oReg);
            Fault initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const override;
            Fault completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const override;
            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
        };
        template class Pstrr<uint8_t>;
        template class Pstrr<uint16_t>;
        template class Pstrr<uint32_t>;
        template class Pstrr<uint64_t>;
    }
}