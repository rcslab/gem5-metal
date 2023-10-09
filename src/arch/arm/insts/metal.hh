#ifndef __ARCH_ARM_INSTS_METAL_HH__
#define __ARCH_ARM_INSTS_METAL_HH__

#include "arch/arm/insts/static_inst.hh"

namespace gem5
{

    namespace ArmISA
    {

        static constexpr std::string_view MetalDisasmPrefix = "Metal: ";
        // Metal instructions with an immediate (menter)
        class MetalImmOp : public ArmStaticInst
        {
        protected:
            uint8_t imm;

        public:
            MetalImmOp(const char *mnem, ExtMachInst _machInst, OpClass __opClass,
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

        // Metal instructions with args (rmr, wmr)
        class MetalRegOp : public ArmStaticInst
        {
        private:
            RegId srcRegIdxArr[1];
            RegId destRegIdxArr[1];
        protected:
            RegIndex mReg;
            RegIndex gReg;

        public:
            MetalRegOp(const char *mnem, ExtMachInst _machInst, OpClass __opClass, RegIndex _mReg, RegIndex _gReg) : ArmStaticInst(mnem, _machInst, __opClass), mReg(_mReg), gReg(_gReg)
            {
                setRegIdxArrays(
                reinterpret_cast<RegIdArrayPtr>(
                    &std::remove_pointer_t<decltype(this)>::srcRegIdxArr),
                reinterpret_cast<RegIdArrayPtr>(
                    &std::remove_pointer_t<decltype(this)>::destRegIdxArr));
        
                setSrcRegIdx(_numSrcRegs++, gem5::ArmISA::couldBeZero(gReg) ? RegId() : intRegClass[gReg]);
                setDestRegIdx(_numDestRegs++, gem5::ArmISA::couldBeZero(gReg) ? RegId() : intRegClass[gReg]);
                _numTypedDestRegs[intRegClass.type()]++;

                this->flags[IsMetal] = true;
            }

            std::string generateDisassembly(
                Addr pc, const loader::SymbolTable *symtab) const override;
        };

        // menter
        class Menter64 : public MetalImmOp
        {
        public:
            Menter64(ExtMachInst _machInst, uint8_t _imm);

            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
            Fault initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const override;
            Fault completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const override;
        };

        // mexit
        class Mexit64 : public MetalNakedOp
        {
        public:
            Mexit64(ExtMachInst _machInst);

            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
        };

        // rmr
        class Rmr64 : public MetalRegOp
        {
        private:
            RegId srcRegIdxArr[1];
            RegId destRegIdxArr[1]; 
        public:
            Rmr64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg);

            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
        };

        // wmr
        class Wmr64 : public MetalRegOp
        {
        private:
            RegId srcRegIdxArr[1];
            RegId destRegIdxArr[1];
        public:
            Wmr64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg);

            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;
        };

    } // namespace ArmISA
} // namespace gem5

#endif //__ARCH_ARM_INSTS_METAL_HH__
