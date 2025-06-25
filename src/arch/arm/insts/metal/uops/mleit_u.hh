#pragma once

#include "arch/arm/insts/metal/common.hh"
#include "arch/generic/memhelpers.hh"

namespace gem5 {
namespace ArmISA {
namespace metal { namespace inst {
    class Mleit64_u : public MetalMicroInst
    {
    protected:
        uint32_t offset;
        uint32_t size;
    public:
        Mleit64_u(ExtMachInst _machInst,
                uint32_t _offset, uint32_t _size) :
                MetalMicroInst("mleit_u", _machInst, MemReadOp), offset(_offset), size(_size)
        {
            this->flags[IsLoad] = true;
        }

        Fault initiateAcc(ExecContext *xc, trace::InstRecord *traceData) const override
        {
            ThreadContext *tc = xc->tcBase();
            const RegVal meb = tc->readMetalMiscRegNoEffect(reg::MEB);
            const Addr base = purifyTaggedAddr(this->offset + meb, tc, currEL(tc), true);

            // no permission check here because wmcr_u already checks it

            METAL_DBGPRINT(INSTS, MLEIT_U, "Loading exc intercept table at 0x%lx + 0x%lx, size %u.\n", meb, this->offset, this->size);

            Fault fault = initiateMemRead(xc, base, this->size, ArmISA::MMU::AllowUnaligned);
            if (traceData) {
                traceData->setMem(base, this->size, ArmISA::MMU::AllowUnaligned);
            }

            return NoFault;
        }

        Fault completeAcc(Packet *pkt, ExecContext *xc, trace::InstRecord *traceData) const override
        {
            ThreadContext *tc = xc->tcBase();
            ISA * isa = static_cast<ISA *>(tc->getIsaPtr());

            if (pkt->isError()) {
                panic("Data fetch failed.");
            }

            static char buf[ExcInterceptTableLoadSize];
            assert(this->size <= ExcInterceptTableLoadSize);
            getMemRawPtr(pkt, buf, this->size, traceData);
            isa->loadExcInterceptTable(buf, this->size);
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
}}
}
}
