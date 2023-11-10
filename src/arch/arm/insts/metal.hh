#ifndef __ARCH_ARM_INSTS_METAL_HH__
#define __ARCH_ARM_INSTS_METAL_HH__

#include "arch/arm/insts/static_inst.hh"
#include "debug/Metal.hh"

namespace gem5
{

    namespace ArmISA
    {
        #define METAL_STR(x) #x
        #define METAL_STR2(x) METAL_STR(x)
        #define METAL_DBGPRINT(subsys, subsys2, format, ...) DPRINTF(Metal, "Metal." METAL_STR2(subsys) "." METAL_STR2(subsys2) ": " format, ##__VA_ARGS__)

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
                    insertBits(data, 63, 48, asid);
                }

                uint16_t vmid() {
                    return bits(data, 47, 32);
                }
                void set_vmid(uint16_t vmid) {
                    insertBits(data, 47, 32, vmid);
                }

                uint8_t attr() {
                    return bits(data, 31, 24);
                }
                void set_attr(uint8_t attr) {
                    insertBits(data, 31, 24, attr);
                }

                ExceptionLevel el() {
                    return (ExceptionLevel)bits(data, 23, 22);
                }
                void set_el(ExceptionLevel el) {
                    insertBits(data, 23, 22, el);
                }

                TlbEntry::MemoryType mtype() {
                    return (TlbEntry::MemoryType)bits(data, 21, 20);
                }
                void set_mtype(TlbEntry::MemoryType mtype) {
                    insertBits(data, 21, 20, mtype);
                }

                bool isHyp() {
                    return bits(data, 19);
                }
                void set_isHyp(bool isHyp) {
                    insertBits(data, 19, isHyp);
                }

                bool isSecure() {
                    return bits(data, 18);
                }
                void set_isSecure(bool isSecure) {
                    insertBits(data, 18, isSecure);
                }

                bool type() {
                    return bits(data, 17);
                }
                void set_type(bool type) {
                    insertBits(data, 17, type);
                }

                bool nc() {
                    return bits(data, 16);
                }
                void set_nc(bool nc) {
                    insertBits(data, 16, nc);
                }
        };

        // Metal instructions with an immediate (menter)
        class MetalImmOp8 : public ArmStaticInst
        {
        protected:
            uint8_t imm;

        public:
            MetalImmOp8(const char *mnem, ExtMachInst _machInst, OpClass __opClass,
                       uint8_t _imm) : ArmStaticInst(mnem, _machInst, __opClass), imm(_imm)
            {
                this->flags[IsMetal] = true;
            }

            std::string generateDisassembly(
                Addr pc, const loader::SymbolTable *symtab) const override;
        };

        // Metal instructions with no args (mexit)
        class MetalNakedOp : public ArmStaticInst
        {
        public:
            MetalNakedOp(const char *mnem, ExtMachInst _machInst, OpClass __opClass) : ArmStaticInst(mnem, _machInst, __opClass)
            {
                this->flags[IsMetal] = true;
            }

            std::string generateDisassembly(
                Addr pc, const loader::SymbolTable *symtab) const override;
        };

        // Metal instructions with 1 reg arg (rar, war)
        class MetalRegOp : public ArmStaticInst
        {
        protected:
            RegId srcRegIdxArr[MAX_METAL_OPERANDS];
            RegId destRegIdxArr[MAX_METAL_OPERANDS];
            RegIndex mReg;

        public:
            MetalRegOp(const char *mnem, ExtMachInst _machInst, OpClass __opClass, RegIndex _mReg) : ArmStaticInst(mnem, _machInst, __opClass), mReg(_mReg)
            {
                setRegIdxArrays(
                reinterpret_cast<RegIdArrayPtr>(
                    &std::remove_pointer_t<decltype(this)>::srcRegIdxArr),
                reinterpret_cast<RegIdArrayPtr>(
                    &std::remove_pointer_t<decltype(this)>::destRegIdxArr));

                this->flags[IsMetal] = true;
            }

            std::string generateDisassembly(
                Addr pc, const loader::SymbolTable *symtab) const override;
        };

        // Metal instructions with 2 reg args (rmr, wmr)
        class MetalRegOp2 : public ArmStaticInst
        {
        protected:
            RegId srcRegIdxArr[MAX_METAL_OPERANDS];
            RegId destRegIdxArr[MAX_METAL_OPERANDS];
            RegIndex mReg;
            RegIndex gReg;

        public:
            MetalRegOp2(const char *mnem, ExtMachInst _machInst, OpClass __opClass, RegIndex _mReg, RegIndex _gReg) : ArmStaticInst(mnem, _machInst, __opClass), mReg(_mReg), gReg(_gReg)
            {
                setRegIdxArrays(
                reinterpret_cast<RegIdArrayPtr>(
                    &std::remove_pointer_t<decltype(this)>::srcRegIdxArr),
                reinterpret_cast<RegIdArrayPtr>(
                    &std::remove_pointer_t<decltype(this)>::destRegIdxArr));

                this->flags[IsMetal] = true;
            }

            std::string generateDisassembly(
                Addr pc, const loader::SymbolTable *symtab) const override;
        };

        // Metal instructions with 3 args (rtlb64, wtlb64)
        class MetalThreeRegOp : public ArmStaticInst
        {
        protected:
            RegIndex rl;
            RegIndex rm;
            RegIndex rn;

        public:
            MetalThreeRegOp(const char *mnem, ExtMachInst _machInst,
                OpClass __opClass, RegIndex _rl, RegIndex _rm, RegIndex _rn)
                : ArmStaticInst(mnem, _machInst, __opClass), rl(_rl),
                rm(_rm), rn(_rn)
            {
                this->flags[IsMetal] = true;
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
            static void doMenter(ThreadContext *xc, Addr npc, Addr lpc);
            static void doMenter(ThreadContext *xc, Addr npc, const ArmStaticInst &inst);
        private:
            static void calcLoadAddr(Addr base, unsigned long align, unsigned int idx, Addr & _loadAddr, unsigned int & _count);
        };

        // mexit
        class Mexit64 : public MetalImmOp8
        {
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
            Fault initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const override;
            Fault completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const override;
        };

        // Read Architectural Register 
        class Rar64 : public MetalRegOp2
        {
        public:
            Rar64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg);

            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
        };

        // rtlb
        class Rtlb64 : public MetalThreeRegOp
        {
        public:
            Rtlb64(ExtMachInst _machInst, RegIndex _rl, RegIndex _rm,
                   RegIndex _rn);

            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
        };

        // Write Architectural Register 
        class War64 : public MetalRegOp2
        {
        public:
            War64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg);

            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
        };

        // wrtlb
        class Wtlb64 : public MetalThreeRegOp
        {
        public:
            Wtlb64(ExtMachInst _machInst, RegIndex _rl, RegIndex _rm,
                   RegIndex _rn);

            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
        };

    } // namespace ArmISA
} // namespace gem5

#endif //__ARCH_ARM_INSTS_METAL_HH__
