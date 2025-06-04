#pragma once

#include "arch/arm/insts/metal/common.hh"
#include "cpu/metal_int_state.hh"
#include "arch/generic/memhelpers.hh"

namespace gem5 {
namespace ArmISA {

    class Mlmrt64_u : public MetalMicroInst
    {
    private:
        uint32_t offset;
        uint32_t size;
        uint32_t startIdx;
    public:
        Mlmrt64_u(ExtMachInst _machInst, OpClass __opClass,
                    uint32_t _offset, uint32_t _size, uint32_t _startIdx) :
        MetalMicroInst("mlmrt_u", _machInst, __opClass), offset(_offset), size(_size), startIdx(_startIdx)
        {
            this->flags[IsLoad] = true;
        }

        Fault initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const override
        {
            ThreadContext * tc = xc->tcBase();
            const RegVal mbr = xc->tcBase()->readMetalMiscRegNoEffect(metal_reg::MBR);

            METAL_DBGPRINT(INSTS, MLMRT_U, "Loading mroutine table at 0x%lx + 0x%lx, size %u, startIdx %d.\n", mbr, offset, this->size, this->startIdx);

            Fault fault = initiateMemRead(xc, purifyTaggedAddr(mbr + offset, tc, currEL(tc), true), this->size, ArmISA::MMU::AllowUnaligned);

            return NoFault;
        }

        Fault completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const override
        {
            ThreadContext *tc = xc->tcBase();
            ISA * isa = static_cast<ISA *>(tc->getIsaPtr());

            if (pkt->isError()) {
                panic("Data fetch failed.");
            }

            static char buf[ISA::MroutineTableLoadSize];
            assert(this->size <= ISA::MroutineTableLoadSize);
            getMemRawPtr(pkt, buf, size, traceData);

            isa->loadMroutineTable(buf, size / sizeof(ISA::MroutineTableEntry), startIdx);
            return NoFault;
        }

        Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override
        {
            panic("unimplemented");
        }

        std::string generateDisassembly(
            Addr pc, const loader::SymbolTable *symtab) const override
        {
            std::stringstream ss;
            ss << MetalDisasmPrefix;
            printMnemonic(ss, "", false);
            ccprintf(ss, "0x%x, 0x%x, %d", this->offset, this->size, this->startIdx);
            return ss.str();
        }
    };

}
}