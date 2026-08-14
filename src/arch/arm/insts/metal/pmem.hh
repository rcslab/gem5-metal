#pragma once

#include "arch/arm/insts/metal/common.hh"
#include "arch/arm/types.hh"
#include "cpu/exec_context.hh"

namespace gem5 {
    namespace ArmISA {
        namespace metal {
            namespace inst {
                template<typename T>
                class MetalPMemOp : public MetalReg3Op {
                protected:
                    bool is_write;

                    MetalPMemOp(const char * mnem, ExtMachInst _machInst, RegIndex _r1, RegIndex _r2, RegIndex _r3, bool _is_write);
                    Fault sendReq(ExecContext *xc, Addr paddr, T * data, size_t data_len,trace::InstRecord *traceData) const;
                    Fault recvResp(Packet *pkt, ExecContext *xc, T * data, size_t data_len, trace::InstRecord *traceData) const;

                    Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
                    std::string generateDisassembly(Addr pc, const loader::SymbolTable *symtab) const override;
                };
                extern template class MetalPMemOp<uint8_t>;
                extern template class MetalPMemOp<uint16_t>;
                extern template class MetalPMemOp<uint32_t>;
                extern template class MetalPMemOp<uint64_t>;
            }
        }
    }
}
