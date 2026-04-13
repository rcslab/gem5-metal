#include "arch/arm/insts/metal/common.hh"
#include "arch/arm/insts/metal/pmem.hh"
#include "base/cprintf.hh"

namespace gem5 { namespace ArmISA {
namespace metal { namespace inst {

        std::string
        MetalImmOp::generateDisassembly(
            Addr pc, const loader::SymbolTable *symtab) const
        {
            std::stringstream ss;
            ss << MetalDisasmPrefix;
            printMnemonic(ss, "", false);
            ss << (unsigned int)this->imm;
            return ss.str();
        }

        std::string
        MetalNakedOp::generateDisassembly(
            Addr pc, const loader::SymbolTable *symtab) const
        {
            std::stringstream ss;
            ss << MetalDisasmPrefix;
            printMnemonic(ss, "", false);
            return ss.str();
        }

        std::string
        MetalRegOp::generateDisassembly(
            Addr pc, const loader::SymbolTable *symtab) const
        {
            std::stringstream ss;
            ss << MetalDisasmPrefix;
            printMnemonic(ss, "", false);
            printIntReg(ss, reg, 64);
            return ss.str();
        }

        std::string
        MetalMRegRegOp::generateDisassembly(
            Addr pc, const loader::SymbolTable *symtab) const
        {
            std::stringstream ss;
            ss << MetalDisasmPrefix;
            printMnemonic(ss, "", false);
            printMetalReg(ss, mReg);
            ccprintf(ss, ", ");
            printIntReg(ss, gReg, 64);
            return ss.str();
        }


        std::string MetalReg2Op::generateDisassembly(
                Addr pc, const loader::SymbolTable *symtab) const
        {
            std::stringstream ss;
            ss << MetalDisasmPrefix;
            printMnemonic(ss, "", false);
            printIntReg(ss, r1, 64);
            ccprintf(ss, ", ");
            printIntReg(ss, r2, 64);
            return ss.str();
        };

        std::string
        MetalReg3Op::generateDisassembly(
            Addr pc, const loader::SymbolTable *symtab) const
        {
            std::stringstream ss;
            ss << MetalDisasmPrefix;
            printMnemonic(ss, "", false);
            printIntReg(ss, r1, 64);
            ccprintf(ss, ", ");
            printIntReg(ss, r2, 64);
            ccprintf(ss, ", ");
            printIntReg(ss, r3, 64);
            return ss.str();
        }

        std::string MetalRegImm2Op::generateDisassembly(
                Addr pc, const loader::SymbolTable *symtab) const
        {
            std::stringstream ss;
            ss << MetalDisasmPrefix;
            printMnemonic(ss, "", false);
            printIntReg(ss, reg, 64);
            ccprintf(ss, ", ");
            printIntReg(ss, imm1, 64);
            ccprintf(ss, ", %d", imm2);
            return ss.str();
        }

        template<typename T>
        std::string MetalPMemOp<T>::generateDisassembly(
                Addr pc, const loader::SymbolTable *symtab) const
        {
            std::stringstream ss;
            ss << MetalDisasmPrefix;
            printMnemonic(ss, "", false);
            printIntReg(ss, r1, 64);
            ccprintf(ss, ", ");
            printIntReg(ss, r2, 64);
            ccprintf(ss, ", ");
            printIntReg(ss, r3, 64);
            return ss.str();
        }
    }}
} // namespace ArmISA
} // namespace gem5
