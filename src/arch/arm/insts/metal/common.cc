#include "arch/arm/insts/metal/common.hh"
#include "arch/arm/regs/metal.hh"
#include "arch/arm/isa.hh"
#include "base/cprintf.hh"

namespace gem5
{
    namespace ArmISA
    {
        std::string
        MetalImmOp8::generateDisassembly(
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
            printMetalReg(ss, mReg);
            return ss.str();
        }

        std::string
        MetalRegOp2::generateDisassembly(
            Addr pc, const loader::SymbolTable *symtab) const
        {
            std::stringstream ss;
            ss << MetalDisasmPrefix;
            printMnemonic(ss, "", false);
            printMetalReg(ss, mReg);
            ccprintf(ss, ", ");
            printIntReg(ss, gReg);
            return ss.str();
        }

        std::string
        MetalRegOp3::generateDisassembly(
            Addr pc, const loader::SymbolTable *symtab) const
        {
            std::stringstream ss;
            ss << MetalDisasmPrefix;
            printMnemonic(ss, "", false);
            printMetalReg(ss, rl);
            ccprintf(ss, ", ");
            printIntReg(ss, rm);
            ccprintf(ss, ", ");
            printIntReg(ss, rn);
            return ss.str();
        }

        std::string MetalPMemRegOp::generateDisassembly(Addr pc, const loader::SymbolTable *symtab) const
        {
            std::stringstream ss;
            ss << MetalDisasmPrefix;
            printMnemonic(ss, "", false);
            printIntReg(ss, rl);
            ccprintf(ss, ", [");
            printIntReg(ss, rm);
            ccprintf(ss, ", ");
            printIntReg(ss, rn);
            ccprintf(ss, "]");
            return ss.str();
        }

        std::string MetalPMemRegImmOp::generateDisassembly(Addr pc, const loader::SymbolTable *symtab) const
        {
            std::stringstream ss;
            ss << MetalDisasmPrefix;
            printMnemonic(ss, "", false);
            printIntReg(ss, mReg);
            ccprintf(ss, ", ");
            switch(mode) {
                case Mode::NORMAL:
                case Mode::PREINDEX:
                    ccprintf(ss, "[");
                    printIntReg(ss, gReg);
                    ccprintf(ss, ", ");
                    ccprintf(ss, "#%#x]", imm);
                    if (mode == Mode::PREINDEX) {
                        ccprintf(ss, "!");
                    }
                    break;
                case Mode::POSTINDEX:
                    ccprintf(ss, "[");
                    printIntReg(ss, gReg);
                    ccprintf(ss, "], #%#x", imm);
                    break;
                default:
                    panic("Unknown Metal PMem mode: %d", static_cast<int>(this->mode));
            }
            return ss.str();
        }
    } // namespace ArmISA
} // namespace gem5
