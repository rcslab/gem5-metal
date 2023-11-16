#ifndef __ARCH_ARM_INSTS_METAL_HH__
#define __ARCH_ARM_INSTS_METAL_HH__

#include "arch/arm/insts/static_inst.hh"
#include "arch/arm/insts/macromem.hh"
#include "debug/Metal.hh"

namespace gem5
{

    namespace ArmISA
    {
        static constexpr size_t MAX_METAL_OPERANDS = 4;
        static constexpr std::string_view MetalDisasmPrefix = "";

        class TLBEntryInfo {
            public:
                uint64_t data;

                TLBEntryInfo() : data(0) {}

                uint16_t asid() {
                    return bits(data, 63, 48);
                }
                void set_asid(uint16_t asid) {
                    data = insertBits(data, 63, 48, asid);
                }

                uint16_t vmid() {
                    return bits(data, 47, 32);
                }
                void set_vmid(uint16_t vmid) {
                    data = insertBits(data, 47, 32, vmid);
                }

                uint8_t attr() {
                    return bits(data, 31, 24);
                }
                void set_attr(uint8_t attr) {
                    data = insertBits(data, 31, 24, attr);
                }

                ExceptionLevel el() {
                    return (ExceptionLevel)bits(data, 23, 22);
                }
                void set_el(ExceptionLevel el) {
                    data = insertBits(data, 23, 22, el);
                }

                TlbEntry::MemoryType mtype() {
                    return (TlbEntry::MemoryType)bits(data, 21, 20);
                }
                void set_mtype(TlbEntry::MemoryType mtype) {
                    data = insertBits(data, 21, 20, mtype);
                }

                bool isHyp() {
                    return bits(data, 19);
                }
                void set_isHyp(bool isHyp) {
                    data = insertBits(data, 19, isHyp);
                }

                bool isSecure() {
                    return bits(data, 18);
                }
                void set_isSecure(bool isSecure) {
                    data = insertBits(data, 18, isSecure);
                }

                bool type() {
                    return bits(data, 17);
                }
                void set_type(bool type) {
                    data = insertBits(data, 17, type);
                }

                bool nc() {
                    return bits(data, 16);
                }
                void set_nc(bool nc) {
                    data = insertBits(data, 16, nc);
                }
        };

        // a Metal instruction that can be either a regular op, a micro op or a macro op
        class MetalStaticInst : public PredOp
        {
        private:
            std::vector<StaticInstPtr> uops;
        protected:
            void clearMicroOps(void)
            {
                uops.clear();
            }
            void addMicroOps(StaticInstPtr inst)
            {
                uops.push_back(inst);
            }
            void finalizeMicroOps(void)
            {
                assert(this->uops.size() > 0);
                uops.at(0)->setFirstMicroop();
                for (size_t i = 0; i < this->uops.size() - 1; i++) {
                    uops.at(i)->setDelayedCommit();
                }
                uops.at(this->uops.size() - 1)->setLastMicroop();
            }
        public:
            MetalStaticInst(const char *mnem, ExtMachInst _machInst, OpClass __opClass) : PredOp(mnem, _machInst, __opClass)
            {
                this->flags[IsMetal] = true;
            }

            ~MetalStaticInst()
            {
                clearMicroOps();
            }

            void
            advancePC(PCStateBase &pcState) const override
            {
                auto &apc = pcState.as<PCState>();
                if (flags[IsLastMicroop]) {
                    apc.uEnd();
                } else if (flags[IsMicroop]) {
                    apc.uAdvance();
                } else {
                    apc.advance();
                }
            }

            void
            advancePC(ThreadContext *tc) const override
            {
                PCState pc = tc->pcState().as<PCState>();
                if (flags[IsLastMicroop]) {
                    pc.uEnd();
                } else if (flags[IsMicroop]) {
                    pc.uAdvance();
                } else {
                    pc.advance();
                }
                tc->pcState(pc);
            }

            StaticInstPtr
            fetchMicroop(MicroPC microPC) const override
            {
                assert(flags[IsMacroop] && uops.size() > 0 && microPC < uops.size() );
                return uops.at(microPC);
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
            RegId srcRegIdxArr[MAX_METAL_OPERANDS];
            RegId destRegIdxArr[MAX_METAL_OPERANDS];
            RegIndex mReg;

        public:
            MetalRegOp(const char *mnem, ExtMachInst _machInst, OpClass __opClass, RegIndex _mReg) : MetalStaticInst(mnem, _machInst, __opClass), mReg(_mReg)
            {
                setRegIdxArrays(
                reinterpret_cast<RegIdArrayPtr>(
                    &std::remove_pointer_t<decltype(this)>::srcRegIdxArr),
                reinterpret_cast<RegIdArrayPtr>(
                    &std::remove_pointer_t<decltype(this)>::destRegIdxArr));
            }

            std::string generateDisassembly(
                Addr pc, const loader::SymbolTable *symtab) const override;
        };

        // Metal instructions with 2 reg args (rmr, wmr)
        class MetalRegOp2 : public MetalStaticInst
        {
        protected:
            RegId srcRegIdxArr[MAX_METAL_OPERANDS];
            RegId destRegIdxArr[MAX_METAL_OPERANDS];
            RegIndex mReg;
            RegIndex gReg;

        public:
            MetalRegOp2(const char *mnem, ExtMachInst _machInst, OpClass __opClass, RegIndex _mReg, RegIndex _gReg) : MetalStaticInst(mnem, _machInst, __opClass), mReg(_mReg), gReg(_gReg)
            {
                setRegIdxArrays(
                reinterpret_cast<RegIdArrayPtr>(
                    &std::remove_pointer_t<decltype(this)>::srcRegIdxArr),
                reinterpret_cast<RegIdArrayPtr>(
                    &std::remove_pointer_t<decltype(this)>::destRegIdxArr));
            }

            std::string generateDisassembly(
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

            std::string generateDisassembly(
                Addr pc, const loader::SymbolTable *symtab) const override;
        };

        // menter
        class Menter64 : public MetalImmOp8
        {
        public:
            Menter64(ExtMachInst _machInst, uint8_t _imm);

            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
            Fault initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const override;
            Fault completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const override;
            static void doMenter(ThreadContext *xc, Addr npc, Addr lpc, bool intr);
            static void doMenter(ThreadContext *xc, Addr npc, const ArmStaticInst &inst, bool intr);
        private:
            static void calcLoadAddr(Addr base, unsigned long align, unsigned int idx, Addr & _loadAddr, unsigned int & _count);
        };

        // mexit
        class Mexit64 : public MetalImmOp8
        {
        private:
            BitUnion8(MexitFlags)
            Bitfield<1> iim; // mask instruction intercept for the next inst
            Bitfield<0> rfi; // this is a return from intercept (Inst & Exc) mroutine (restore CPSR from MSPSR)
            EndBitUnion(MexitFlags)

        public:
            Mexit64(ExtMachInst _machInst, uint8_t _imm);

            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
        };

        // rmr
        class Rmr64 : public MetalRegOp2
        {
        public:
            Rmr64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg);

            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
        };

        // wmr
        class Wmr64 : public MetalRegOp2
        {
        public:
            Wmr64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg);

            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
        };

        // Read Architectural Register
        class Rar64 : public MetalRegOp2
        {
        public:
            Rar64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg);

            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
        };

        // Write Architectural Register
        class War64 : public MetalRegOp2
        {
        public:
            War64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg);

            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
        };

        // rpr
        class Rpr64 : public MetalRegOp2
        {
        public:
            Rpr64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg);

            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
        };

        // wpr
        class Wpr64 : public MetalRegOp2
        {
        public:
            Wpr64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg);

            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
        };

        // rtlb
        class Rtlb64 : public MetalRegOp3
        {
        public:
            Rtlb64(ExtMachInst _machInst, RegIndex _rl, RegIndex _rm,
                   RegIndex _rn);

            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
        };

        // wrtlb
        class Wtlb64 : public MetalRegOp3
        {
        public:
            Wtlb64(ExtMachInst _machInst, RegIndex _rl, RegIndex _rm,
                   RegIndex _rn);

            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
        };

        // msti
        class Msti64 : public MetalNakedOp
        {
        public:
            Msti64(ExtMachInst _machInst);
            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
        };

        // mcli
        class Mcli64 : public MetalNakedOp
        {
        public:
            Mcli64(ExtMachInst _machInst);
            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
        };

        // microops
        class Mliit64_u : public MetalNakedOp
        {
        protected:
            uint32_t offset;
            uint32_t size;
        public:
            Mliit64_u(ExtMachInst _machInst, OpClass __opClass,
                       uint32_t _offset, uint32_t _size);

            Fault initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const override;
            Fault completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const override;
            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
            std::string generateDisassembly(
                Addr pc, const loader::SymbolTable *symtab) const override;
        };

        class Mleit64_u : public MetalNakedOp
        {
        protected:
            uint32_t offset;
            uint32_t size;
        public:
            Mleit64_u(ExtMachInst _machInst, OpClass __opClass,
                       uint32_t _offset, uint32_t _size);

            Fault initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const override;
            Fault completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const override;
            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
            std::string generateDisassembly(
                Addr pc, const loader::SymbolTable *symtab) const override;
        };

        class Wmr64_u : public MetalRegOp2
        {
        public:
            Wmr64_u(ExtMachInst _machInst, OpClass __opClass, RegIndex mReg, RegIndex gReg);
            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
        };
    } // namespace ArmISA
} // namespace gem5

#endif //__ARCH_ARM_INSTS_METAL_HH__
