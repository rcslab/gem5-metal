#pragma once

#include "arch/arm/insts/metal/pmem.hh"

namespace gem5 {
    namespace ArmISA {
    namespace metal { namespace inst {
        template <typename T>
        class Pstr : public MetalPMemOp<T>
        {
            static_assert(isPowerOf2(sizeof(T)) && (sizeof(T) <= sizeof(RegVal)));
        public:
            Pstr(ExtMachInst _machInst, RegIndex _dReg, RegIndex _bReg, RegIndex _oReg);
            Fault initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const override;
            Fault completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const override;
        };
        template class Pstr<uint8_t>;
        template class Pstr<uint16_t>;
        template class Pstr<uint32_t>;
        template class Pstr<uint64_t>;

        template <typename T>
        class Pstp : public MetalPMemOp<T>
        {
            static_assert(isPowerOf2(sizeof(T)) && (sizeof(T) <= sizeof(RegVal)));
        public:
            Pstp(ExtMachInst _machInst, RegIndex _dReg, RegIndex _bReg, RegIndex _oReg);
            Fault initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const override;
            Fault completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const override;
        };
        template class Pstp<uint32_t>;
        template class Pstp<uint64_t>;
    }}
    }
}
