#pragma once
#include "arch/arm/insts/metal/common.hh"

namespace gem5 {
    namespace ArmISA {
    namespace metal { namespace inst {
        class Rmgr64 : public MetalMRegRegOp
        {
        public:
            Rmgr64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg);

            Fault execute(ExecContext *xc, trace::InstRecord *traceData) const override;

            std::string generateDisassembly(
                Addr pc, const loader::SymbolTable *symtab) const override;
        };
    }}
    }
}
