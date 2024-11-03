#pragma once

#include "arch/arm/insts/metal/common.hh"

namespace gem5 {
    namespace ArmISA {
        class Rmcr64 : public MetalRegOp2
        {
        public:
            Rmcr64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg);

            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;

            std::string generateDisassembly(
                Addr pc, const loader::SymbolTable *symtab) const override;
        };
    }
}