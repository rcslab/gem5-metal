#include "arch/arm/insts/metal/pldr.hh"
#include "arch/generic/memhelpers.hh"

namespace gem5 {
    namespace ArmISA {
    namespace metal { namespace inst {
        static constexpr const char * MNEM_LOOKUP_TABLE[] = {"pldrb", "pldrh", "pldrw", "pldr"};

        template <typename T>
        Pldri<T>::Pldri(ExtMachInst _machInst, RegIndex _dReg, RegIndex _sReg, int32_t _imm, Mode _mode) : 
            MetalPMemRegImmOp(MNEM_LOOKUP_TABLE[log2i(sizeof(T))] , _machInst, MemReadOp, _dReg, _sReg, _imm, _mode)
        {
            setSrcRegIdx(_numSrcRegs++, intRegClass[_sReg]);
            setDestRegIdx(_numDestRegs++, intRegClass[_dReg]);
            setDestRegIdx(_numDestRegs++, intRegClass[_sReg]);
            _numTypedDestRegs[intRegClass.type()] += 2;

            this->flags[IsLoad] = true;
        }

        template <typename T>
        Fault Pldri<T>::initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const
        {
            const gem5::metal::InternalState & mist = xc->getMetalState();

            if (mist.getLevel() == 0) {
                METAL_DBGPRINT(INSTS, PLDR, "Permission denied: MetalState = [%s].\n", mist.toStr().c_str());
                return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }

            Addr base = xc->getRegOperand(this, 0);
            METAL_DBGPRINT(INSTS, PLDRI, "dReg = %u, sReg = %u, imm = %d, addr = %#lx, mode = %#x, size = %u.\n",
                    r1, r2, imm, base + imm,
                    static_cast<int>(mode) ,
                    sizeof(T));

            switch (mode) {
                case Mode::PREINDEX:
                    base = base + imm;
                    xc->setRegOperand(this, 1, base);
                    break;
                case Mode::NORMAL:
                    base = base + imm;
                    break;
                default:
                    break;
            }

            Fault fault = NoFault;

            fault = initiateMemRead(xc, base, sizeof(T), ArmISA::MMU::AllowUnaligned | ArmISA::MMU::BypassMMU);
            if(traceData)
                traceData->setMem(base, sizeof(T), ArmISA::MMU::AllowUnaligned | ArmISA::MMU::BypassMMU);
            return fault;
        }

        template <typename T>
        Fault Pldri<T>::completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const
        {
            if (pkt->isError()) {
                panic("Data fetch failed.");
            }

            T mem;
            if (isBigEndian64(xc->tcBase())) {
                getMem<ByteOrder::big, T>(pkt, mem, traceData);
            } else {
                getMem<ByteOrder::little, T>(pkt, mem, traceData);
            }

            xc->setRegOperand(this, 0, static_cast<RegVal>(mem));

            if (mode == Mode::POSTINDEX) {
                xc->setRegOperand(this, 1, xc->getRegOperand(this, 0) + imm);
            }

            return NoFault;
        }

        template <typename T>
        Fault Pldri<T>::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            panic("unimplemented.");
        }


        // pldrr
        template <typename T>
        Pldrr<T>::Pldrr(ExtMachInst _machInst, RegIndex _dReg, RegIndex _bReg, RegIndex _oReg) :
            MetalPMemRegOp(MNEM_LOOKUP_TABLE[log2i(sizeof(T))], _machInst, MemReadOp, _dReg, _bReg, _oReg)
        {
            setSrcRegIdx(_numSrcRegs++, intRegClass[_bReg]);
            setSrcRegIdx(_numSrcRegs++, intRegClass[_oReg]);
            setDestRegIdx(_numDestRegs++, intRegClass[_dReg]);
            _numTypedDestRegs[intRegClass.type()]++;

            this->flags[IsLoad] = true;
        }

        template <typename T>
        Fault Pldrr<T>::initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const
        {
            const gem5::metal::InternalState & mist = xc->getMetalState();
            const Addr addr = xc->getRegOperand(this, 0) + xc->getRegOperand(this, 1);

            METAL_DBGPRINT(INSTS, PLDRR, "dReg = %u, bReg = %u, oReg = %u, addr = %#lx, size = %u.\n",
                    r1, r2, r3, addr,
                    sizeof(T));

            if (mist.getLevel() <= 0) {
                METAL_DBGPRINT(INSTS, PLDR, "Permission denied: MetalState = [%s].\n", mist.toStr().c_str());
                return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }


            Fault fault = initiateMemRead(xc, addr, sizeof(T), ArmISA::MMU::AllowUnaligned | ArmISA::MMU::BypassMMU);

            return fault;
        }

        template <typename T>
        Fault Pldrr<T>::completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const
        {
            if (pkt->isError()) {
                panic("Data fetch failed.");
            }

            T mem;
            if (isBigEndian64(xc->tcBase())) {
                getMem<ByteOrder::big, T>(pkt, mem, traceData);
            } else {
                getMem<ByteOrder::little, T>(pkt, mem, traceData);
            }

            xc->setRegOperand(this, 0, static_cast<RegVal>(mem));

            return NoFault;
        }

        template <typename T>
        Fault Pldrr<T>::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            panic("unimplemented.");
        }
    }}
    }
}
