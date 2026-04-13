#include "arch/arm/insts/metal/pmem.hh"
#include "arch/arm/insts/metal/common.hh"
#include "arch/arm/utility.hh"
#include "arch/generic/memhelpers.hh"
#include "base/types.hh"
#include "enums/ByteOrder.hh"
#include "sim/byteswap.hh"

namespace gem5 {
    namespace ArmISA {
    namespace metal { namespace inst {
        template<typename T>
        MetalPMemOp<T>::MetalPMemOp(const char * mnem, ExtMachInst _machInst, RegIndex _r1, RegIndex _r2, RegIndex _r3, bool _is_write) :
            MetalReg3Op(mnem, _machInst, _is_write ? OpClass::MemWrite : OpClass::MemRead, _r1, _r2, _r3), is_write(_is_write)
        {
            if(is_write) {
                this->flags[IsStore] = true;
            } else {
                this->flags[IsLoad] = true;
            }
        }

        template<typename T>
        Fault MetalPMemOp<T>::sendReq(ExecContext *xc, Addr paddr, T * data, size_t data_len, trace::InstRecord *traceData) const
        {
            Fault fault = NoFault;
            const ByteOrder order = isBigEndian64(xc->tcBase()) ? ByteOrder::big : ByteOrder::little;

            if (is_write) {
                assert(data != nullptr);

                for (int i = 0; i < data_len; i++) {
                    data[i] = htog<T>(data[i], order);
                }

                const std::vector<bool> byte_enable(sizeof(T) * data_len, true);

                fault = writeMemTiming(xc,
                    reinterpret_cast<uint8_t*>(data),
                    paddr,
                    sizeof(*data) * data_len,
                    ArmISA::MMU::AllowUnaligned | ArmISA::MMU::BypassMMU,
                    nullptr,
                    byte_enable);

            } else {
                fault = initiateMemRead(xc, paddr, sizeof(*data) * data_len, ArmISA::MMU::AllowUnaligned | ArmISA::MMU::BypassMMU);
                if(traceData)
                    traceData->setMem(paddr, sizeof(*data) * data_len, ArmISA::MMU::AllowUnaligned | ArmISA::MMU::BypassMMU);
            }
            return fault;
        }

        template<typename T>
        Fault MetalPMemOp<T>::recvResp(Packet *pkt, ExecContext *xc, T * data, size_t data_len, trace::InstRecord *traceData) const
        {
            Fault fault = NoFault;
            if (pkt->isError()) {
                panic("Data fetch failed.");
            }

            if (!is_write) {
                assert(data != nullptr);

                const ByteOrder order = isBigEndian64(xc->tcBase()) ? ByteOrder::big : ByteOrder::little;
                getMemRawPtr(pkt, data, sizeof(*data) * data_len, traceData);

                for (int i = 0; i < data_len; i++) {
                    data[i] = gtoh<T>(data[i], order);
                }
            }

            return fault;
        }

        template<typename T>
        Fault MetalPMemOp<T>::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            panic("unimplemented.");
        }
    }}
    }
}
