#pragma once


#include "arch/arm/insts/metal/common.hh"
#include "cpu/metal_int_state.hh"
#include "arch/generic/memhelpers.hh"

namespace gem5 {
namespace ArmISA {
    class Mliit64_u : public MetalMicroInst
    {
    protected:
        uint32_t offset;
        uint32_t size;
    public:
        Mliit64_u(ExtMachInst _machInst, uint32_t _offset, uint32_t _size) :
        MetalMicroInst("mliit_u", _machInst, MemReadOp), offset(_offset), size(_size)
        {
            this->flags[IsLoad] = true;
        }

        Fault initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const override
        {
            ThreadContext *tc = xc->tcBase();
            const RegVal mib = tc->readMetalMiscRegNoEffect(metal_reg::MIB);
            const Addr base = purifyTaggedAddr(this->offset + mib, tc, currEL(tc), true);

            // no permission check here because wmcr_u already checks it
            METAL_DBGPRINT(INSTS, MLIIT_U, "Loading inst intercept table at 0x%lx + 0x%lx, size %u.\n", mib, this->offset, this->size);

            Fault fault = initiateMemRead(xc, base, this->size, ArmISA::MMU::AllowUnaligned);
            if(traceData)
                traceData->setMem(base, this->size, ArmISA::MMU::AllowUnaligned | ArmISA::MMU::BypassMMU);
            return NoFault;
        }

        Fault completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const override
        {
            ThreadContext *tc = xc->tcBase();
            ISA * isa = static_cast<ISA *>(tc->getIsaPtr());
            RegVal mib = tc->readMetalMiscRegNoEffect(metal_reg::MIB);

            if (pkt->isError()) {
                panic("Data fetch failed.");
            }

            static char buf[ISA::InstInterceptTableLoadSize];
            assert(this->size <= ISA::InstInterceptTableLoadSize);
            getMemRawPtr(pkt, buf, this->size, traceData);
            isa->loadInstInterceptTable(buf, purifyTaggedAddr(this->offset + mib, tc, currEL(tc), true), this->size);
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
            ccprintf(ss, "0x%x, 0x%x", this->offset, this->size);
            return ss.str();
        }
    };
}
}
