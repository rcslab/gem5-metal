#include "arch/arm/insts/metal/wmcr.hh"
#include "arch/generic/memhelpers.hh"
#include "arch/arm/insts/metal/uops/mleit_u.hh"
#include "arch/arm/insts/metal/uops/mlmrt_u.hh"
#include "arch/arm/insts/metal/uops/wmcr_u.hh"
#include "arch/arm/insts/metal/uops/mliit_u.hh"

namespace gem5 {
    namespace ArmISA {
        // wmr
        Wmcr64::Wmcr64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg) : 
            MetalMacroInst("wmcr", _machInst, IntAluOp), mReg(_mreg), gReg(_greg)
        {
            this->flags[IsInteger] = true;
            this->flags[IsSerializeAfter] = true;
            this->flags[IsNonSpeculative] = true;

            this->numMicroops = 1;

            if (mReg == metal_reg::MIB) {
                this->flags[IsLoad] = true;
                this->numMicroops += ISA::InstInterceptTableTotalSize / ISA::InstInterceptTableLoadSize;
            } else if (mReg == metal_reg::MEB) {
                this->flags[IsLoad] = true;
                this->numMicroops += ISA::ExcInterceptTableTotalSize / ISA::ExcInterceptTableLoadSize;
            } else if (mReg == metal_reg::MBR) {
                this->flags[IsLoad] = true;
                this->numMicroops += ISA::MroutineTableTotalSize / ISA::MroutineTableLoadSize;
            }
            
            this->microOps = new StaticInstPtr[this->numMicroops];
            StaticInst * inst = new Wmcr64_u(_machInst, _opClass, mReg, gReg);
            this->microOps[0] = inst;

            if (mReg == metal_reg::MIB) {
                for (int i = 0; i < ISA::InstInterceptTableTotalSize / ISA::InstInterceptTableLoadSize; i++) {
                    StaticInstPtr uop = new Mliit64_u(_machInst, _opClass, i * ISA::InstInterceptTableLoadSize, ISA::InstInterceptTableLoadSize);
                    this->microOps[i+1] = uop;
                }
            } else if (mReg == metal_reg::MEB) {
                for (int i = 0; i < ISA::ExcInterceptTableTotalSize / ISA::ExcInterceptTableLoadSize; i++) {
                    StaticInstPtr uop = new Mleit64_u(_machInst, _opClass, i * ISA::ExcInterceptTableLoadSize, ISA::ExcInterceptTableLoadSize);
                    this->microOps[i+1] = uop;
                }
            } else if (mReg == metal_reg::MBR) {
                for (int i = 0; i < ISA::MroutineTableTotalSize / ISA::MroutineTableLoadSize; i++) {
                    StaticInstPtr uop = new Mlmrt64_u(_machInst, _opClass, i * ISA::MroutineTableLoadSize, ISA::MroutineTableLoadSize, 
                            i * (ISA::MroutineTableLoadSize / sizeof(ISA::MroutineTableEntry)));
                    this->microOps[i+1] = uop;
                }
            }
            this->microOps[this->numMicroops - 1]->setFlag(IsSerializeAfter);
            this->microOps[this->numMicroops - 1]->setFlag(IsNonSpeculative);
            this->finalize();
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