#include "arch/arm/insts/metal/wmr.hh"
#include "cpu/metal_int_state.hh"

namespace gem5 {
    namespace ArmISA {
    namespace metal { namespace inst {
        // wmr
        Wmr64::Wmr64(ExtMachInst _machInst, RegIndex _mreg, RegIndex _greg) : MetalMRegRegOp("wmr", _machInst, IntAluOp, _mreg, _greg)
        {
            setSrcRegIdx(_numSrcRegs++, intRegClass[gReg]);
            setDestRegIdx(_numDestRegs++, metalRegClass[mReg]);
            _numTypedDestRegs[metalRegClass.type()]++;
            this->flags[IsInteger] = true;
        }

        Fault Wmr64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            const auto & state = xc->getMetalState();
            RegVal v = xc->getRegOperand(this, 0);

            METAL_DBGPRINT(INSTS, WMR, "%s @ Lv.%d => 0x%lx.\n", printMetalReg(mReg), state.getLevel(), v);

            xc->setRegOperand(this, 0, v);
            if (traceData)
                traceData->setData(metalRegClass, v);

            return NoFault;
        }
    }}
    }
}
