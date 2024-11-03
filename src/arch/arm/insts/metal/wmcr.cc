#include "arch/arm/insts/metal/wmcr.hh"
#include "arch/generic/memhelpers.hh"

namespace gem5 {
    namespace ArmISA {

        class Wmcr64_u : public MetalRegOp2
        {
        public:
            Wmcr64_u(ExtMachInst _machInst, OpClass __opClass, RegIndex _mReg, RegIndex _gReg) :
                MetalRegOp2("wmcr_u", _machInst, __opClass, _mReg, _gReg)
            {
                setSrcRegIdx(_numSrcRegs++, gem5::ArmISA::couldBeZero(gReg) ? RegId() : intRegClass[gReg]);
                setDestRegIdx(_numDestRegs++, metalMiscRegClass[mReg]);

                _numTypedDestRegs[metalMiscRegClass.type()]++;

                this->flags[IsMicroop] = true;
                this->flags[IsInteger] = true;
            }
    
            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override 
            {
                ThreadContext * tc = xc->tcBase();
                metal_reg::MSR_t msr = tc->readMetalMiscRegNoEffect(metal_reg::MSR);
                const RegVal mar = tc->readMetalMiscRegNoEffect(metal_reg::MAR);
                RegVal v = xc->getRegOperand(this, 0);

                bool allowWrite = false;
                if (!msr.init && mReg == metal_reg::MBR) {
                    // allow Metal initialization
                    allowWrite = true;
                    msr.init = 1;
                    tc->setMetalMiscReg(metal_reg::MSR, msr);
                } else {

                    allowWrite = metal_reg::getWritePerm(mar, mReg) && metal_reg::isInMetalMode(msr);
                }

                if (!allowWrite) {
                    METAL_DBGPRINT(INSTS, WMCR_U, "Permission denied: writing %s (MAR = 0x%lx, MSR = 0x%lx).\n", printMetalMiscReg(mReg), mar, msr);
                    return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
                }

                switch(this->mReg) {
                    case metal_reg::MSR: {
                        metal_reg::MSR_t new_val = v;
                        // msr.lv is readonly
                        new_val.lv = msr.lv;
                        // msr.init is readonly
                        new_val.init = msr.init;
                        v = new_val;
                        break;
                    }
                    
                    default: {
                        break;
                    }
                }

                METAL_DBGPRINT(INSTS, WMCR_U, "%s => 0x%lx.\n", printMetalMiscReg(mReg), v);
                tc->setMetalMiscReg(mReg, v);

                return NoFault;
            }
        };


        // microops
        class Mliit64_u : public MetalNakedOp
        {
        protected:
            uint32_t offset;
            uint32_t size;
        public:
            Mliit64_u(ExtMachInst _machInst, OpClass __opClass,
                       uint32_t _offset, uint32_t _size) :
            MetalNakedOp("mliit_u", _machInst, __opClass), offset(_offset), size(_size)
            {
                this->flags[IsMicroop] = true;
                this->flags[IsLoad] = true;
                this->flags[IsInteger] = true;
            }

            Fault initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const override 
            {
                ThreadContext *tc = xc->tcBase();
                metal_reg::MSR_t msr = tc->readMetalMiscRegNoEffect(metal_reg::MSR);
                RegVal mib = tc->readMetalMiscRegNoEffect(metal_reg::MIB);

                // no permission check here because wmcr_u already checks it
                METAL_DBGPRINT(INSTS, MLIIT_U, "Loading inst intercept table at 0x%lx + 0x%lx, size %u.\n", mib, this->offset, this->size);

                Fault fault = initiateMemRead(xc, purifyTaggedAddr(this->offset + mib, tc, currEL(tc), true), this->size, ArmISA::MMU::AllowUnaligned);

                return NoFault;
            }

            Fault completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const override 
            {
                ThreadContext *tc = xc->tcBase();
                ISA * isa = static_cast<ISA *>(tc->getIsaPtr());
                RegVal mib = tc->readMetalMiscRegNoEffect(metal_reg::MIB);

                if (pkt->isError()) {
                    panic("Data fetch failed.");
                }

                static char buf[ISA::InstInterceptTableLoadSize];
                assert(this->size <= ISA::InstInterceptTableLoadSize);
                getMemRawPtr(pkt, buf, this->size, traceData);
                isa->loadInstInterceptTable(buf, purifyTaggedAddr(this->offset + mib, tc, currEL(tc), true), this->size);
                return NoFault;
            }

            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override 
            {
                panic("unimplemented");
            }

            std::string generateDisassembly(
                Addr pc, const loader::SymbolTable *symtab) const override
            {
                std::stringstream ss;
                ss << MetalDisasmPrefix;
                printMnemonic(ss, "", false);
                ccprintf(ss, "0x%x, 0x%x", this->offset, this->size);
                return ss.str();
            }
        };

        class Mleit64_u : public MetalNakedOp
        {
        protected:
            uint32_t offset;
            uint32_t size;
        public:
            Mleit64_u(ExtMachInst _machInst, OpClass __opClass,
                       uint32_t _offset, uint32_t _size) :
            MetalNakedOp("mleit_u", _machInst, __opClass), offset(_offset), size(_size)
            {
                this->flags[IsMicroop] = true;
                this->flags[IsLoad] = true;
                this->flags[IsInteger] = true;
            }

            Fault initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const override
            {
                ThreadContext *tc = xc->tcBase();
                metal_reg::MSR_t msr = tc->readMetalMiscRegNoEffect(metal_reg::MSR);
                RegVal meb = tc->readMetalMiscRegNoEffect(metal_reg::MEB);

                // no permission check here because wmcr_u already checks it

                METAL_DBGPRINT(INSTS, MLEIT_U, "Loading exc intercept table at 0x%lx + 0x%lx, size %u.\n", meb, this->offset, this->size);

                Fault fault = initiateMemRead(xc, purifyTaggedAddr(this->offset + meb, tc, currEL(tc), true), this->size, ArmISA::MMU::AllowUnaligned);

                return NoFault;
            }

            Fault completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const override
            {
                ThreadContext *tc = xc->tcBase();
                ISA * isa = static_cast<ISA *>(tc->getIsaPtr());
                RegVal meb = tc->readMetalMiscReg(metal_reg::MEB);

                if (pkt->isError()) {
                    panic("Data fetch failed.");
                }

                static char buf[ISA::ExcInterceptTableLoadSize];
                assert(this->size <= ISA::ExcInterceptTableLoadSize);
                getMemRawPtr(pkt, buf, this->size, traceData);
                isa->loadExcInterceptTable(buf, purifyTaggedAddr(this->offset + meb, tc, currEL(tc), true), this->size);
                return NoFault;
            }

            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override
            {
                panic("unimplemented");
            }

            std::string generateDisassembly(
                Addr pc, const loader::SymbolTable *symtab) const override
            {
                std::stringstream ss;
                ss << MetalDisasmPrefix;
                printMnemonic(ss, "", false);
                ccprintf(ss, "0x%x, 0x%x", this->offset, this->size);
                return ss.str();
            }
        };

        // wmr
        Wmcr64::Wmcr64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg) : MetalRegOp2("wmcr", _machInst, IntAluOp, _mreg, _greg)
        {
            setSrcRegIdx(_numSrcRegs++, gem5::ArmISA::couldBeZero(gReg) ? RegId() : intRegClass[gReg]);
            setDestRegIdx(_numDestRegs++, metalRegClass[mReg]);
            _numTypedDestRegs[metalRegClass.type()]++;
            this->flags[IsMacroop] = true;
            this->flags[IsInteger] = true;
            StaticInstPtr uop;

            uop = new Wmcr64_u(_machInst, _opClass, mReg, gReg);
            this->addMicroOps(uop);
            if (mReg == metal_reg::MIB) {
                this->flags[IsLoad] = true;
                for (int i = 0; i < ISA::InstInterceptTableTotalSize / ISA::InstInterceptTableLoadSize; i++) {
                    uop = new Mliit64_u(_machInst, _opClass, i * ISA::InstInterceptTableLoadSize, ISA::InstInterceptTableLoadSize);
                    this->addMicroOps(uop);
                }
            } else if (mReg == metal_reg::MEB) {
                this->flags[IsLoad] = true;
                for (int i = 0; i < ISA::ExcInterceptTableTotalSize / ISA::ExcInterceptTableLoadSize; i++) {
                    uop = new Mleit64_u(_machInst, _opClass, i * ISA::ExcInterceptTableLoadSize, ISA::ExcInterceptTableLoadSize);
                    this->addMicroOps(uop);
                }
            }
            this->finalizeMicroOps();
        }

        Fault Wmcr64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            panic("unimplemented!");
        }

        std::string Wmcr64::generateDisassembly(
            Addr pc, const loader::SymbolTable *symtab) const
        {
            std::stringstream ss;
            ss << MetalDisasmPrefix;
            printMnemonic(ss, "", false);
            printMetalMiscReg(ss, mReg);
            ccprintf(ss, ", ");
            printIntReg(ss, gReg);
            return ss.str();
        }
    }
}