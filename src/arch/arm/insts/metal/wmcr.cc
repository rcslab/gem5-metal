#include "arch/arm/insts/metal/wmcr.hh"
#include "arch/arm/insts/metal/uops/mleit_u.hh"
#include "arch/arm/insts/metal/uops/mlmrt_u.hh"
#include "arch/arm/insts/metal/uops/msinit_u.hh"
#include "arch/arm/insts/metal/uops/wmcr_u.hh"
#include "arch/arm/insts/metal/uops/mliit_u.hh"
#include "arch/arm/regs/metal_misc.hh"
#include "enums/StaticInstFlags.hh"

namespace gem5 {
    namespace ArmISA {
    namespace metal { namespace inst {
        // wmr
        Wmcr64::Wmcr64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg) :
            MetalMacroInst("wmcr", _machInst, IntAluOp), mReg(_mreg), gReg(_greg)
        {
            this->flags[IsInteger] = true;
            this->flags[IsSerializeAfter] = true;
            this->flags[IsNonSpeculative] = true;

            this->numMicroops = 1;

            if (mReg == reg::MIB) {
                this->flags[IsLoad] = true;
                this->numMicroops += InstInterceptTableTotalSize / InstInterceptTableLoadSize;
            } else if (mReg == reg::MEB) {
                this->flags[IsLoad] = true;
                this->numMicroops += ExcInterceptTableTotalSize / ExcInterceptTableLoadSize;
            } else if (mReg == reg::MBR) {
                this->flags[IsLoad] = true;
                this->numMicroops += MroutineTableTotalSize / MroutineTableLoadSize;
            }

            if (reg::isSetInit(mReg)) {
                this->numMicroops++;
            }

            this->microOps = new StaticInstPtr[this->numMicroops];
            int curMicroOp = 0;
            this->microOps[curMicroOp++] = new Wmcr64_u(_machInst, mReg, gReg);

            if (mReg == reg::MIB) {
                for (int i = 0; i < InstInterceptTableTotalSize / InstInterceptTableLoadSize; i++) {
                    StaticInstPtr uop = new Mliit64_u(_machInst, i * InstInterceptTableLoadSize, InstInterceptTableLoadSize);
                    this->microOps[curMicroOp++] = uop;
                }
            } else if (mReg == reg::MEB) {
                for (int i = 0; i < ExcInterceptTableTotalSize / ExcInterceptTableLoadSize; i++) {
                    StaticInstPtr uop = new Mleit64_u(_machInst, i * ExcInterceptTableLoadSize, ExcInterceptTableLoadSize);
                    this->microOps[curMicroOp++] = uop;
                }
            } else if (mReg == reg::MBR) {
                for (int i = 0; i < MroutineTableTotalSize / MroutineTableLoadSize; i++) {
                    StaticInstPtr uop = new Mlmrt64_u(_machInst, i * MroutineTableLoadSize, MroutineTableLoadSize,
                            i * (MroutineTableLoadSize / sizeof(MroutineTableEntry)));
                    this->microOps[curMicroOp++] = uop;
                }
            }

            if (reg::isSetInit(mReg)) {
                this->microOps[curMicroOp++] = new Msinit64_u(_machInst);
            }

            assert(curMicroOp == this->numMicroops);
            this->microOps[curMicroOp - 1]->setFlag(IsSerializeAfter);
            this->microOps[curMicroOp - 1]->setFlag(IsNonSpeculative);
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
    }}
    }
}
