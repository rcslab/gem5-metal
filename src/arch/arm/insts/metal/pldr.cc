#include "arch/arm/insts/metal/pldr.hh"
#include "arch/generic/memhelpers.hh"

namespace gem5 {
    namespace ArmISA {
        static constexpr size_t log2(size_t n)
        {
            return ( (n<2) ? 0 : 1 + log2(n/2));
        }
        static constexpr const char * MNEM_LOOKUP_TABLE[] = {"pldrb", "pldrh", "pldrw", "pldr"};

        template <typename T>
        Pldri<T>::Pldri(ExtMachInst _machInst, RegIndex _dReg, RegIndex _sReg, int32_t _imm, Mode _mode) : 
            MetalPMemRegImmOp(MNEM_LOOKUP_TABLE[log2(sizeof(T))] , _machInst, MemReadOp, _dReg, _sReg, _imm, _mode)
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
            const MetalInternalState & mist = xc->getMetalState();

            metal_reg::MSR_t msr = mist.getMSR();

            if (!metal_reg::isInMetalMode(msr)) {
                METAL_DBGPRINT(INSTS, PLDR, "Permission denied: MSR = 0x%lx.\n", msr);
                return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }

            Addr base = xc->getRegOperand(this, 0);
            METAL_DBGPRINT(INSTS, PLDRI, "dReg = %u, sReg = %u, imm = %d, mode = %#x, size = %u.\n",
                    r1, r2, imm,
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
            MetalPMemRegOp(MNEM_LOOKUP_TABLE[log2(sizeof(T))], _machInst, MemReadOp, _dReg, _bReg, _oReg)
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
            const MetalInternalState & mist = xc->getMetalState();
            const metal_reg::MSR_t msr = mist.getMSR();
            const Addr addr = xc->getRegOperand(this, 0) + xc->getRegOperand(this, 1);

            METAL_DBGPRINT(INSTS, PLDRR, "dReg = %u, bReg = %u, oReg = %u, addr = %#lx, size = %u.\n",
                    r1, r2, r3, addr,
                    sizeof(T));

            if (!metal_reg::isInMetalMode(msr)) {
                METAL_DBGPRINT(INSTS, PLDR, "Permission denied: MSR = 0x%lx.\n", msr);
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
    }
}
