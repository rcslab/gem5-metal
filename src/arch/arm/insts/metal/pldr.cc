#include "arch/arm/insts/metal/pldr.hh"
#include "arch/generic/memhelpers.hh"
#include "mem/packet_access.hh"

namespace gem5 {
    namespace ArmISA {
        template <typename T>
        Pldri<T>::Pldri(ExtMachInst _machInst, RegIndex _dReg, RegIndex _sReg, int32_t _imm, Mode _mode) :
            MetalPMemRegImmOp("pldr", _machInst, MemReadOp, _dReg, _sReg, _imm, _mode)
        {
            setSrcRegIdx(_numSrcRegs++, intRegClass[_sReg]);
            setDestRegIdx(_numDestRegs++, intRegClass[_dReg]);
            setDestRegIdx(_numDestRegs++, intRegClass[_sReg]);
            _numTypedDestRegs[intRegClass.type()] += 2;

            this->flags[IsInteger] = true;
            this->flags[IsLoad] = true;
        }

        template <typename T>
        Fault Pldri<T>::initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const
        {
            ThreadContext * tc = xc->tcBase();
            metal_reg::MSR_t msr = tc->readMetalMiscRegNoEffect(metal_reg::MSR);
            Addr base = xc->getRegOperand(this, 0);

            METAL_DBGPRINT(INSTS, PLDRI, "dReg = %u, sReg = %u, imm = %d, mode = %#x, size = %u.\n",
                    mReg, gReg, imm,
                    static_cast<int>(mode) ,
                    sizeof(T));

            if (!metal_reg::isInMetalMode(msr)) {
                METAL_DBGPRINT(INSTS, PLDR, "Permission denied: MSR = 0x%lx.\n", msr);
                return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }

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
            MetalPMemRegOp("pldr", _machInst, MemReadOp, _dReg, _bReg, _oReg)
        {
            setSrcRegIdx(_numSrcRegs++, intRegClass[_bReg]);
            setSrcRegIdx(_numSrcRegs++, intRegClass[_oReg]);
            setDestRegIdx(_numDestRegs++, intRegClass[_dReg]);
            _numTypedDestRegs[intRegClass.type()]++;

            this->flags[IsInteger] = true;
            this->flags[IsLoad] = true;
        }

        template <typename T>
        Fault Pldrr<T>::initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const
        {
            ThreadContext * tc = xc->tcBase();
            metal_reg::MSR_t msr = tc->readMetalMiscRegNoEffect(metal_reg::MSR);
            const Addr addr = xc->getRegOperand(this, 0) + xc->getRegOperand(this, 1);

            METAL_DBGPRINT(INSTS, PLDRR, "dReg = %u, bReg = %u, oReg = %u, addr = %#lx, size = %u.\n",
                    rl, rm, rn, addr,
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
