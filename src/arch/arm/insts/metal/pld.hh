#pragma once

#include "arch/arm/insts/metal/pmem.hh"

namespace gem5 {
    namespace ArmISA {
        namespace metal { namespace inst {
        template <typename T>
        class Pldr : public MetalPMemOp<T>
        {
            static_assert(isPowerOf2(sizeof(T)) && (sizeof(T) <= sizeof(RegVal)));
        public:
            Pldr(ExtMachInst _machInst, RegIndex _dReg, RegIndex _bReg, RegIndex _oReg);
            Fault initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const override;
            Fault completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const override;
        };
        template class Pldr<uint8_t>;
        template class Pldr<uint16_t>;
        template class Pldr<uint32_t>;
        template class Pldr<uint64_t>;

        template <typename T>
        class Pldp : public MetalPMemOp<T>
        {
            static_assert(isPowerOf2(sizeof(T)) && (sizeof(T) <= sizeof(RegVal)));
        public:
            Pldp(ExtMachInst _machInst, RegIndex _dReg, RegIndex _bReg, RegIndex _oReg);
            Fault initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const override;
            Fault completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const override;
        };
        template class Pldp<uint32_t>;
        template class Pldp<uint64_t>;
    }}
    }
}
