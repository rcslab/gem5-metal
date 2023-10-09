#include "arch/arm/insts/metal.hh"
#include "arch/arm/regs/metal.hh"
#include "arch/arm/isa.hh"
#include "arch/arm/pcstate.hh"
#include "arch/generic/memhelpers.hh"
#include "debug/Metal.hh"
#include "mem/packet_access.hh"
#include "mem/request.hh"

#include "base/cprintf.hh"

namespace gem5
{

    namespace ArmISA
    {

        std::string
        MetalImmOp::generateDisassembly(
            Addr pc, const loader::SymbolTable *symtab) const
        {
            std::stringstream ss;
            ss << MetalDisasmPrefix;
            printMnemonic(ss, "", false);
            ss << this->imm;
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
            printMetalReg(ss, gReg);
            ccprintf(ss, ", ");
            printMetalReg(ss, mReg);
            return ss.str();
        }

        // menter
        Menter64::Menter64(ExtMachInst _machInst, uint8_t _imm) : MetalImmOp("menter", _machInst, IntAluOp, _imm)
        {
            this->flags[IsCall] = true;
            this->flags[IsControl] = true;
            this->flags[IsDirectControl] = true;
            this->flags[IsInteger] = true;
            this->flags[IsUncondControl] = true;
            this->flags[IsLoad] = true;
        }

        Fault Menter64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            // get mroutine table's base address
            const RegVal base = xc->readMetalReg(metal_reg::MBR);

            const Addr mroutine_ea = base + sizeof(uintptr_t) * this->imm;

            DPRINTF(Metal, "MENTER: MBR = 0x%x, idx = %d\n", base, this->imm);

            uintptr_t target;

            Fault fault = readMemAtomicLE(xc, traceData, mroutine_ea, target, ArmISA::MMU::AllowUnaligned);
            target = cSwap(target, isBigEndian64(xc->tcBase()));

            if (fault != NoFault)
            {
                return fault;
            }

            // set new PC
            const Addr target_addr = purifyTaggedAddr(target, xc->tcBase(), currEL(xc->tcBase()), true);
            PCState pcState;
            set(pcState, xc->pcState());
            pcState.instNPC(target_addr);
            xc->pcState(pcState);

            // save link address
            xc->setMetalReg(metal_reg::MLR, pcState.pc() + 4);
            // increase metal level
            xc->setMetalReg(metal_reg::MSR, xc->readMetalReg(metal_reg::MSR) + 1);

            return fault;
        }

        Fault Menter64::initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const
        {
            // get mroutine table's base address
            const RegVal base = xc->readMetalReg(metal_reg::MBR);
            const Addr mroutine_ea = base + sizeof(uintptr_t) * this->imm;

            uintptr_t target;

            DPRINTF(Metal, "MENTER: MBR = 0x%x, idx = %d\n", base, this->imm);

            Fault fault = initiateMemRead(xc, traceData, mroutine_ea, target, ArmISA::MMU::AllowUnaligned);

            return fault;
        }

        Fault Menter64::completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const
        {
            uintptr_t target;
            getMemLE(pkt, target, traceData);
            target = cSwap(target, isBigEndian64(xc->tcBase()));

            // set new PC
            const Addr target_addr = purifyTaggedAddr(target, xc->tcBase(), currEL(xc->tcBase()), true);
            PCState pcState;
            set(pcState, xc->pcState());
            pcState.instNPC(target_addr);
            xc->pcState(pcState);

            // save link address
            xc->setMetalReg(metal_reg::MLR, pcState.pc() + 4);
            // increase metal level
            xc->setMetalReg(metal_reg::MSR, xc->readMetalReg(metal_reg::MSR) + 1);

            return NoFault;
        }

        // mexit
        Mexit64::Mexit64(ExtMachInst _machInst) : MetalNakedOp("mexit", _machInst, IntAluOp)
        {
            this->flags[IsControl] = true;
            this->flags[IsIndirectControl] = true;
            this->flags[IsInteger] = true;
            this->flags[IsReturn] = true;
            this->flags[IsUncondControl] = true;
        }

        Fault Mexit64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            // get mroutine table's base address
            const RegVal ret = xc->readMetalReg(metal_reg::MLR);
            DPRINTF(Metal, "MEXIT: MLR = 0x%x\n", ret);

            // set new PC
            const Addr target_addr = purifyTaggedAddr(ret, xc->tcBase(), currEL(xc->tcBase()), true);
            PCState pcState;
            set(pcState, xc->pcState());
            pcState.instNPC(target_addr);
            xc->pcState(pcState);

            return NoFault;
        }

        // wmr
        Wmr64::Wmr64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg) : MetalRegOp("wmr", _machInst, IntAluOp, _mreg, _greg)
        {
            this->flags[IsInteger] = true;
        }

        Fault Wmr64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            ThreadContext *tc = xc->tcBase();

            DPRINTF(Metal, "WMR: mReg = %d, gReg = %d\n", mReg, gReg);

            if (!metal_reg::canWriteMetalReg(tc, mReg))
            {
                return std::make_shared<UndefinedInstruction>(machInst, false, mnemonic);
            }

            RegVal v = xc->getRegOperand(this, 1);
            xc->setMetalReg(mReg, v);

            return NoFault;
        }

        // rmr
        Rmr64::Rmr64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg) : MetalRegOp("rmr", _machInst, IntAluOp, _mreg, _greg)
        {
            this->flags[IsInteger] = true;
        }

        Fault Rmr64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            ThreadContext *tc = xc->tcBase();

            DPRINTF(Metal, "RMR: mReg = %d, gReg = %d\n", mReg, gReg);

            if (!metal_reg::canReadMetalReg(tc, mReg))
            {
                return std::make_shared<UndefinedInstruction>(machInst, false, mnemonic);
            }

            RegVal v = xc->readMetalReg(mReg);
            xc->setRegOperand(this, 1, v);

            return NoFault;
        }

    } // namespace ArmISA
} // namespace gem5
