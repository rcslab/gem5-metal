#pragma once

#include "arch/arm/insts/static_inst.hh"
#include "arch/arm/insts/macromem.hh"
#include "arch/arm/regs/metal.hh"
#include "arch/arm/regs/metal_misc.hh"
#include "debug/Metal.hh"

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
        class MetalImmOp8 : public MetalStaticInst
        {
        protected:
            uint8_t imm;

        public:
            MetalImmOp8(const char *mnem, ExtMachInst _machInst, OpClass __opClass,
                       uint8_t _imm) : MetalStaticInst(mnem, _machInst, __opClass), imm(_imm)
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
            RegIndex mReg;

        public:
            MetalRegOp(const char *mnem, ExtMachInst _machInst, OpClass __opClass, RegIndex _mReg) : MetalStaticInst(mnem, _machInst, __opClass), mReg(_mReg)
            {
            }

            std::string generateDisassembly(
                Addr pc, const loader::SymbolTable *symtab) const override;
        };

        // Metal instructions with 2 reg args (rmr, wmr)
        class MetalRegOp2 : public MetalStaticInst
        {
        protected:
            RegIndex mReg;
            RegIndex gReg;

        public:
            MetalRegOp2(const char *mnem, ExtMachInst _machInst, OpClass __opClass, RegIndex _mReg, RegIndex _gReg) : MetalStaticInst(mnem, _machInst, __opClass), mReg(_mReg), gReg(_gReg)
            {
            }

            virtual std::string generateDisassembly(
                Addr pc, const loader::SymbolTable *symtab) const override;
        };

        // Metal instructions with 3 args (rtlb64, wtlb64)
        class MetalRegOp3 : public MetalStaticInst
        {
        protected:
            RegIndex rl;
            RegIndex rm;
            RegIndex rn;
        public:
            MetalRegOp3(const char *mnem, ExtMachInst _machInst,
                OpClass __opClass, RegIndex _rl, RegIndex _rm, RegIndex _rn)
                : MetalStaticInst(mnem, _machInst, __opClass), rl(_rl),
                rm(_rm), rn(_rn)
            {
            }

            virtual std::string generateDisassembly(
                Addr pc, const loader::SymbolTable *symtab) const override;
        };

        class MetalPMemRegOp : public MetalRegOp3
        {
        public:
            MetalPMemRegOp(const char *mnem, ExtMachInst _machInst, OpClass __opClass, RegIndex _dReg, RegIndex _bReg, RegIndex _oReg) :
                MetalRegOp3(mnem, _machInst, __opClass, _dReg, _bReg, _oReg)
            {
            }

            std::string generateDisassembly(
                Addr pc, const loader::SymbolTable *symtab) const override;
        };

        class MetalPMemRegImmOp : public MetalRegOp2
        {
        public:
            enum class Mode {
                NORMAL,
                PREINDEX,
                POSTINDEX
            };
        protected:
            int32_t imm;
            Mode mode;
        public:
            MetalPMemRegImmOp(const char *mnem, ExtMachInst _machInst, OpClass __opClass, RegIndex _mReg, RegIndex _gReg, int32_t _imm, Mode _mode) :
                MetalRegOp2(mnem, _machInst, __opClass, _mReg, _gReg), imm(_imm), mode(_mode)
            {
            }

            std::string generateDisassembly(
                Addr pc, const loader::SymbolTable *symtab) const override;
        };


    } // namespace ArmISA
} // namespace gem5

