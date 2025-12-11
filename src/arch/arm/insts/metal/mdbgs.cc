#include "arch/arm/insts/metal/mdbgs.hh"
#include "arch/arm/regs/metal.hh"
#include "cpu/metal_int_state.hh"

namespace gem5 { namespace ArmISA {
namespace metal{ namespace inst {
        Mdbgs64::Mdbgs64(ExtMachInst _machInst) : MetalNakedOp("mdbgs", _machInst, IntAluOp)
        {
            this->flags[IsSerializeAfter] = true;
            this->flags[IsNonSpeculative] = true;
        }

        Fault Mdbgs64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {

            METAL_DBGPRINT(INSTS, MDBGS64, "Enabling Debug Logging (Metal, Exec)....");

            ObjectMatch activate("Metal");
            ObjectMatch activate2("Exec");

            trace::getDebugLogger()->addActivate(activate);
            trace::getDebugLogger()->addActivate(activate2);

            return NoFault;
        }
}}
}}
