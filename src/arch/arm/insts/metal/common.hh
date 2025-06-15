#pragma once

#include "arch/arm/insts/pred_inst.hh"

namespace gem5
{
    namespace ArmISA
    {
        static constexpr std::string_view MetalDisasmPrefix = "";
        static constexpr size_t MAX_METAL_OPERANDS = 16;

        class MetalStaticInst : public PredOp
        {
        protected:
            RegId srcRegIdxArr[MAX_METAL_OPERANDS];
            RegId destRegIdxArr[MAX_METAL_OPERANDS];
        public:
            MetalStaticInst(const char *mnem, ExtMachInst _machInst, OpClass __opClass) : PredOp(mnem, _machInst, __opClass)
            {
                setRegIdxArrays(
                    reinterpret_cast<RegIdArrayPtr>(
                        &std::remove_pointer_t<decltype(this)>::srcRegIdxArr),
                    reinterpret_cast<RegIdArrayPtr>(
                        &std::remove_pointer_t<decltype(this)>::destRegIdxArr));
            }
        };

        class MetalMacroInst : public MetalStaticInst
        {
        protected:
            uint32_t numMicroops;
            StaticInstPtr * microOps;
            void finalize(void)
            {
                if (numMicroops == 0) {
                    return;
                }

                microOps[numMicroops - 1]->setLastMicroop();
                microOps[0]->setFirstMicroop();
                for(int i = 0; i < numMicroops - 1; i++) {
                    microOps[i]->setDelayedCommit();
                }
            }
        public:
            MetalMacroInst(const char *mnem, ExtMachInst _machInst, OpClass __opClass) :
                MetalStaticInst(mnem, _machInst, __opClass),
                numMicroops(0),
                microOps(nullptr)
            {
                this->flags[IsMacroop] = true;
            }

            ~MetalMacroInst()
            {
                if (microOps)
                    delete [] microOps;
            }

            StaticInstPtr
            fetchMicroop(MicroPC microPC) const override
            {
                assert(microPC < numMicroops);
                return microOps[microPC];
            }

            Fault
            execute(ExecContext *, trace::InstRecord *) const override
            {
                panic("Execute method called when it shouldn't!");
            }

            std::string generateDisassembly(
                    Addr pc, const loader::SymbolTable *symtab) const override
            {
                    std::stringstream ss;

                    ccprintf(ss, "%-10s ", mnemonic);

                    return ss.str();
            }

            void size(size_t newSize) override
            {
                for (int i = 0; i < numMicroops; i++) {
                    microOps[i]->size(newSize);
                }
                _size = newSize;
            }
        };

        class MetalMicroInst : public MetalStaticInst
        {
        public:
            MetalMicroInst(const char *mnem, ExtMachInst _machInst, OpClass __opClass) : MetalStaticInst(mnem, _machInst, __opClass)
            {
                this->flags[IsMicroop] = true;
            }

            void
            advancePC(PCStateBase &pcState) const override
            {
                auto &apc = pcState.as<PCState>();
                if (flags[IsLastMicroop])
                    apc.uEnd();
                else
                    apc.uAdvance();
            }

            void
            advancePC(ThreadContext *tc) const override
            {
                PCState pc = tc->pcState().as<PCState>();
                if (flags[IsLastMicroop])
                    pc.uEnd();
                else
                    pc.uAdvance();
                tc->pcState(pc);
            }
        };

        // Metal instructions with an immediate (menter)
        class MetalImmOp : public MetalStaticInst
        {
        protected:
            uint imm;
        public:
            MetalImmOp(const char *mnem, ExtMachInst _machInst, OpClass __opClass,
                       uint _imm) : MetalStaticInst(mnem, _machInst, __opClass), imm(_imm)
            {
            }

            std::string generateDisassembly(
                Addr pc, const loader::SymbolTable *symtab) const override;
        };

        // Metal instructions with no args (mexit)
        class MetalNakedOp : public MetalStaticInst
        {
        public:
            MetalNakedOp(const char *mnem, ExtMachInst _machInst, OpClass __opClass) : MetalStaticInst(mnem, _machInst, __opClass)
            {
            }

            virtual std::string generateDisassembly(
                Addr pc, const loader::SymbolTable *symtab) const override;
        };

        // Metal instructions with 1 reg arg (rar, war)
        class MetalRegOp : public MetalStaticInst
        {
        protected:
            RegIndex reg;

        public:
            MetalRegOp(const char *mnem, ExtMachInst _machInst, OpClass __opClass, RegIndex _reg) : MetalStaticInst(mnem, _machInst, __opClass), reg(_reg)
            {
            }

            std::string generateDisassembly(
                Addr pc, const loader::SymbolTable *symtab) const override;
        };

        // Metal instructions with 2 reg args (rmr, wmr)
        class MetalMRegRegOp : public MetalStaticInst
        {
        protected:
            RegIndex mReg;
            RegIndex gReg;

        public:
            MetalMRegRegOp(const char *mnem, ExtMachInst _machInst, OpClass __opClass, RegIndex _mReg, RegIndex _gReg) : MetalStaticInst(mnem, _machInst, __opClass), mReg(_mReg), gReg(_gReg)
            {

            }

            virtual std::string generateDisassembly(
                Addr pc, const loader::SymbolTable *symtab) const override;
        };

        class MetalReg2Op : public MetalStaticInst
        {
        protected:
            RegIndex r1;
            RegIndex r2;
        public:
            MetalReg2Op(const char *mnem, ExtMachInst _machInst,
                OpClass __opClass, RegIndex _r1, RegIndex _r2)
                : MetalStaticInst(mnem, _machInst, __opClass), r1(_r1),
                r2(_r2)
            {
            }

            virtual std::string generateDisassembly(
                Addr pc, const loader::SymbolTable *symtab) const override;
        };

        class MetalReg3Op : public MetalStaticInst
        {
        protected:
            RegIndex r1;
            RegIndex r2;
            RegIndex r3;
        public:
            MetalReg3Op(const char *mnem, ExtMachInst _machInst,
                OpClass __opClass, RegIndex _r1, RegIndex _r2, RegIndex _r3)
                : MetalStaticInst(mnem, _machInst, __opClass), r1(_r1),
                r2(_r2), r3(_r3)
            {
            }

            virtual std::string generateDisassembly(
                Addr pc, const loader::SymbolTable *symtab) const override;
        };

        class MetalRegImm2Op : public MetalStaticInst
        {
        protected:
            RegIndex reg;
            uint imm1;
            uint imm2;
        public:
            MetalRegImm2Op(const char *mnem, ExtMachInst _machInst, OpClass __opClass,
                       RegIndex _reg, uint _imm1, uint _imm2) : MetalStaticInst(mnem, _machInst, __opClass), reg(_reg), imm1(_imm1), imm2(_imm2)
            {

            }

            std::string generateDisassembly(
                Addr pc, const loader::SymbolTable *symtab) const override;
        };

        class MetalPMemRegOp : public MetalReg3Op
        {
        public:
            MetalPMemRegOp(const char *mnem, ExtMachInst _machInst, OpClass __opClass, RegIndex _dReg, RegIndex _bReg, RegIndex _oReg) :
                MetalReg3Op(mnem, _machInst, __opClass, _dReg, _bReg, _oReg)
            {

            }

            std::string generateDisassembly(
                Addr pc, const loader::SymbolTable *symtab) const override;
        };

        class MetalPMemRegImmOp : public MetalReg2Op
        {
        public:
            enum class Mode {
                NORMAL,
                PREINDEX,
                POSTINDEX
            };
        protected:
            int imm;
            Mode mode;
        public:
            MetalPMemRegImmOp(const char *mnem, ExtMachInst _machInst, OpClass __opClass, RegIndex _r1, RegIndex _r2, int32_t _imm, Mode _mode) :
                MetalReg2Op(mnem, _machInst, __opClass, _r1, _r2), imm(_imm), mode(_mode)
            {

            }

            std::string generateDisassembly(
                Addr pc, const loader::SymbolTable *symtab) const override;
        };
    } // namespace ArmISA
} // namespace gem5
