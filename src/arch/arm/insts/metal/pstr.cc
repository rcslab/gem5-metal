#include "arch/arm/insts/metal/pstr.hh"
#include "arch/generic/memhelpers.hh"

namespace gem5 {
    namespace ArmISA {
        // pstr, imm offset
        template <typename T>
        Pstri<T>::Pstri(ExtMachInst _machInst, RegIndex _sReg, RegIndex _aReg, int32_t _imm, Mode _mode) :
            MetalPMemRegImmOp("pstr", _machInst, MemWriteOp, _sReg, _aReg, _imm, _mode)
        {
            setSrcRegIdx(_numSrcRegs++, intRegClass[_sReg]);
            setSrcRegIdx(_numSrcRegs++, intRegClass[_aReg]);
            setDestRegIdx(_numDestRegs++,intRegClass[_aReg]);
            _numTypedDestRegs[intRegClass.type()]++;

            this->flags[IsInteger] = true;
            this->flags[IsStore] = true;
        }

        template <typename T>
        Fault Pstri<T>::initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const
        {
            ThreadContext * tc = xc->tcBase();
            metal_reg::MSR_t msr = tc->readMetalMiscRegNoEffect(metal_reg::MSR);

            METAL_DBGPRINT(INSTS, PSTRI, "sReg = %u, aReg = %u, imm = %d, mode = %#x, size = %u.\n",
                    mReg, gReg, imm,
                    static_cast<int>(mode),
                    sizeof(T));

            if (!metal_reg::isInMetalMode(msr)) {
                METAL_DBGPRINT(INSTS, PSTR, "Permission denied: MSR = 0x%lx.\n", msr);
                return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }

            Addr base = xc->getRegOperand(this, 1);

            switch (mode) {
                case Mode::PREINDEX:
                    base = base + imm;
                    xc->setRegOperand(this, 0, base);
                    break;
                case Mode::NORMAL:
                    base = base + imm;
                    break;
                default:
                    break;
            }

            Fault fault = NoFault;

            T mem = static_cast<T>(xc->getRegOperand(this, 0));

            if (isBigEndian64(xc->tcBase())) {
                fault = writeMemTimingBE(xc, traceData, mem, base, ArmISA::MMU::AllowUnaligned | ArmISA::MMU::BypassMMU, nullptr);
            } else {
                fault = writeMemTimingLE(xc, traceData, mem, base, ArmISA::MMU::AllowUnaligned | ArmISA::MMU::BypassMMU, nullptr);
            }

            return fault;
        }

        template <typename T>
        Fault Pstri<T>::completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const
        {
            if (pkt->isError()) {
                panic("Data write failed.");
            }

            return NoFault;
        }

        template <typename T>
        Fault Pstri<T>::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            panic("unimplemented.");
        }


        // pstr, reg offset
        template <typename T>
        Pstrr<T>::Pstrr(ExtMachInst _machInst, RegIndex _dReg, RegIndex _bReg, RegIndex _oReg) :
            MetalPMemRegOp("pstr", _machInst, MemWriteOp, _dReg, _bReg, _oReg)
        {
            setSrcRegIdx(_numSrcRegs++, intRegClass[_dReg]);
            setSrcRegIdx(_numSrcRegs++, intRegClass[_bReg]);
            setSrcRegIdx(_numSrcRegs++, intRegClass[_oReg]);

            this->flags[IsInteger] = true;
            this->flags[IsStore] = true;
        }

        template <typename T>
        Fault Pstrr<T>::initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const
        {
            ThreadContext * tc = xc->tcBase();
            metal_reg::MSR_t msr = tc->readMetalMiscRegNoEffect(metal_reg::MSR);
            const Addr addr = xc->getRegOperand(this, 1) + xc->getRegOperand(this, 2);

            METAL_DBGPRINT(INSTS, PSTRR, "sReg = %u, bReg = %u, oReg = %u, addr = %#lx, size = %u.\n",
                    rl, rm, rn, addr,
                    sizeof(T));

            if (!metal_reg::isInMetalMode(msr)) {
                METAL_DBGPRINT(INSTS, PSTR, "Permission denied: MSR = 0x%lx.\n", msr);
                return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }

            Fault fault = NoFault;

            T mem = static_cast<T>(xc->getRegOperand(this, 0));

            if (isBigEndian64(xc->tcBase())) {
                fault = writeMemTimingBE(xc, traceData, mem, addr, ArmISA::MMU::AllowUnaligned | ArmISA::MMU::BypassMMU, nullptr);
            } else {
                fault = writeMemTimingLE(xc, traceData, mem, addr, ArmISA::MMU::AllowUnaligned | ArmISA::MMU::BypassMMU, nullptr);
            }

            return fault;
        }

        template <typename T>
        Fault Pstrr<T>::completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const
        {
            if (pkt->isError()) {
                panic("Data fetch failed.");
            }

            return NoFault;
        }

        template <typename T>
        Fault Pstrr<T>::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            panic("unimplemented.");
        }
    }
}