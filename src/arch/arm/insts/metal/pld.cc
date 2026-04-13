#include "arch/arm/insts/metal/pld.hh"
#include "arch/arm/insts/metal/pmem.hh"
#include "base/types.hh"

namespace gem5 {
    namespace ArmISA {
    namespace metal { namespace inst {
        static constexpr const char * PLDR_MNEM_LOOKUP_TABLE[] = {"pldb", "pldh", "pldw", "pldr"};
        template <typename T>
        Pldr<T>::Pldr(ExtMachInst _machInst, RegIndex _dReg, RegIndex _bReg, RegIndex _oReg) :
            MetalPMemOp<T>(PLDR_MNEM_LOOKUP_TABLE[log2i(sizeof(T))], _machInst, _dReg, _bReg, _oReg, false)
        {
            this->setSrcRegIdx(this->_numSrcRegs++, intRegClass[_bReg]);
            this->setSrcRegIdx(this->_numSrcRegs++, intRegClass[_oReg]);
            this->setDestRegIdx(this->_numDestRegs++, intRegClass[_dReg]);
            this->_numTypedDestRegs[intRegClass.type()]++;
        }

        template <typename T>
        Fault Pldr<T>::initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const
        {
            const gem5::metal::InternalState & mist = xc->getMetalState();
            const Addr addr = xc->getRegOperand(this, 0) + xc->getRegOperand(this, 1);

            METAL_DBGPRINT(INSTS, PLDR, "r1 = %u, r2 = %u, r3 = %u, addr = %#lx, size = %u.\n",
                    this->r1, this->r2, this->r3, addr, sizeof(T));

            if (mist.getLevel() <= 0) {
                METAL_DBGPRINT(INSTS, PLDR, "Permission denied: MetalState = [%s].\n", mist.toStr().c_str());
                return std::make_shared<SupervisorTrap>(this->machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }


            Fault fault = MetalPMemOp<T>::sendReq(xc, addr, nullptr, 1, traceData);

            return fault;
        }

        template <typename T>
        Fault Pldr<T>::completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const
        {
            T data;

            Fault fault = MetalPMemOp<T>::recvResp(pkt, xc, &data, 1, traceData);
            if (fault == NoFault) {
                xc->setRegOperand(this, 0, static_cast<RegVal>(data));
            }

            return fault;
        }

        // pldp
        static constexpr const char * PLDP_MNEM_LOOKUP_TABLE[] = {"pldpb", "pldph", "pldpw", "pldp"};

        template <typename T>
        Pldp<T>::Pldp(ExtMachInst _machInst, RegIndex _r1, RegIndex _r2, RegIndex _r3) :
            MetalPMemOp<T>(PLDP_MNEM_LOOKUP_TABLE[log2i(sizeof(T))], _machInst, _r1, _r2, _r3, false)
        {
            this->setSrcRegIdx(this->_numSrcRegs++, intRegClass[_r3]);
            this->setDestRegIdx(this->_numDestRegs++, intRegClass[_r1]);
            this->setDestRegIdx(this->_numDestRegs++, intRegClass[_r2]);
            this->_numTypedDestRegs[intRegClass.type()] += 2;
        }

        template <typename T>
        Fault Pldp<T>::initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const
        {
            const gem5::metal::InternalState & mist = xc->getMetalState();
            const Addr addr = xc->getRegOperand(this, 0);

            METAL_DBGPRINT(INSTS, PLDP, "r1 = %u, r2 = %u, r3 = %u, addr = %#lx, size = %u.\n",
                    this->r1, this->r2, this->r3, addr, sizeof(T) * 2);

            if (mist.getLevel() <= 0) {
                METAL_DBGPRINT(INSTS, PLDP, "Permission denied: MetalState = [%s].\n", mist.toStr().c_str());
                return std::make_shared<SupervisorTrap>(this->machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }


            Fault fault = MetalPMemOp<T>::sendReq(xc, addr, nullptr, 2, traceData);

            return fault;
        }

        template <typename T>
        Fault Pldp<T>::completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const
        {
            T data[2];

            Fault fault = MetalPMemOp<T>::recvResp(pkt, xc, data, 2, traceData);
            if (fault == NoFault) {
                xc->setRegOperand(this, 0, static_cast<RegVal>(data[0]));
                xc->setRegOperand(this, 1, static_cast<RegVal>(data[1]));
            }

            return fault;
        }

    }}
    }
}
