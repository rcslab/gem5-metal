#include "arch/arm/insts/metal/pst.hh"
#include "arch/arm/insts/metal/pmem.hh"

namespace gem5 {
    namespace ArmISA {
    namespace metal { namespace inst {

        static constexpr const char * PSTR_MNEM_LOOKUP_TABLE[] = {"pstrb", "pstrh", "pstrw", "pstr"};

        template <typename T>
        Pstr<T>::Pstr(ExtMachInst _machInst, RegIndex _dReg, RegIndex _bReg, RegIndex _oReg) :
            MetalPMemOp<T>(PSTR_MNEM_LOOKUP_TABLE[log2i(sizeof(T))], _machInst, _dReg, _bReg, _oReg, true)
        {
            this->setSrcRegIdx(this->_numSrcRegs++, intRegClass[_dReg]);
            this->setSrcRegIdx(this->_numSrcRegs++, intRegClass[_bReg]);
            this->setSrcRegIdx(this->_numSrcRegs++, intRegClass[_oReg]);
        }

        template <typename T>
        Fault Pstr<T>::initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const
        {
            const gem5::metal::InternalState & mist = xc->getMetalState();
            const Addr addr = xc->getRegOperand(this, 1) + xc->getRegOperand(this, 2);

            METAL_DBGPRINT(INSTS, PSTR, "r1 = %u, r2 = %u, r3 = %u, addr = %#lx, size = %u.\n",
                    this->r1, this->r2, this->r3, addr, sizeof(T));

            if (mist.getLevel() <= 0) {
                METAL_DBGPRINT(INSTS, PSTR, "Permission denied: MetalState = [%s].\n", mist.toStr().c_str());
                return std::make_shared<SupervisorTrap>(this->machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }

            T mem = static_cast<T>(xc->getRegOperand(this, 0));
            Fault fault = MetalPMemOp<T>::sendReq(xc, addr, &mem, 1, traceData);

            return fault;
        }

        template <typename T>
        Fault Pstr<T>::completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const
        {
            return MetalPMemOp<T>::recvResp(pkt, xc, nullptr, 1, traceData);
        }


        static constexpr const char * PSTP_MNEM_LOOKUP_TABLE[] = {"pstpb", "pstph", "pstpw", "pstp"};

        template <typename T>
        Pstp<T>::Pstp(ExtMachInst _machInst, RegIndex _r1, RegIndex _r2, RegIndex _r3) :
            MetalPMemOp<T>(PSTP_MNEM_LOOKUP_TABLE[log2i(sizeof(T))], _machInst, _r1, _r2, _r3, true)
        {
            this->setSrcRegIdx(this->_numSrcRegs++, intRegClass[_r1]);
            this->setSrcRegIdx(this->_numSrcRegs++, intRegClass[_r2]);
            this->setSrcRegIdx(this->_numSrcRegs++, intRegClass[_r3]);
        }

        template <typename T>
        Fault Pstp<T>::initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const
        {
            const gem5::metal::InternalState & mist = xc->getMetalState();
            const Addr addr = xc->getRegOperand(this, 2);

            METAL_DBGPRINT(INSTS, PSTP, "r1 = %u, r2 = %u, r3 = %u, addr = %#lx, size = %u.\n",
                    this->r1, this->r2, this->r3, addr, sizeof(T) * 2);

            if (mist.getLevel() <= 0) {
                METAL_DBGPRINT(INSTS, PSTP, "Permission denied: MetalState = [%s].\n", mist.toStr().c_str());
                return std::make_shared<SupervisorTrap>(this->machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }

            T mem[2];
            mem[0] = static_cast<T>(xc->getRegOperand(this, 0));
            mem[1] = static_cast<T>(xc->getRegOperand(this, 1));
            Fault fault = MetalPMemOp<T>::sendReq(xc, addr, mem, 2, traceData);

            return fault;
        }

        template <typename T>
        Fault Pstp<T>::completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const
        {
            return MetalPMemOp<T>::recvResp(pkt, xc, nullptr, 2, traceData);
        }


    }}
    }
}
