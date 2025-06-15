#include "arch/arm/insts/metal/mint.hh"
#include "arch/arm/insts/metal/menter.hh"
#include "arch/arm/regs/metal.hh"
#include "arch/arm/regs/metal_misc.hh"
#include "base/types.hh"

namespace gem5 {
    namespace ArmISA {
        Mint64::Mint64(ExtMachInst _machInst, uint _imm) : Menter64(_machInst, _imm), saved_mflags(0)
        {
            this->flags[IsPreExecOperandUpdate] = true;
        }

        Fault Mint64::preExec(ExecContext *xc, trace::InstRecord *traceData)
        {
            Fault fault;

            if ((fault = Menter64::preExec(xc, traceData)) != NoFault) {
                return fault;
            }

            setDestRegIdx(_numDestRegs++, metalRegClass[metal_reg::MSPSR].flatten(xc));
            _numTypedDestRegs[metalRegClass.type()]++;
            // setDestRegIdx(_numDestRegs++, metalRegClass[metal_reg::MSFLAGS].flatten(xc));
            // _numTypedDestRegs[metalRegClass.type()]++;

            setSrcRegIdx(_numSrcRegs++, ccRegClass[cc_reg::Nz].flatten(xc));
            setSrcRegIdx(_numSrcRegs++, ccRegClass[cc_reg::C].flatten(xc));
            setSrcRegIdx(_numSrcRegs++, ccRegClass[cc_reg::V].flatten(xc));

            return NoFault;
        }

        Fault Mint64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            // save NzCV
            const RegVal nz = xc->getRegOperand(this, 0);
            const RegVal c = xc->getRegOperand(this, 1);
            const RegVal v = xc->getRegOperand(this, 2);

            CPSR mspsr = 0;
            mspsr.nz = nz;
            mspsr.c = c;
            mspsr.v = v;

            // set mspsr
            xc->setRegOperand(this, 1, mspsr);

            // set msflags
            // xc->setRegOperand(this, 2, saved_mflags);

            METAL_DBGPRINT(INSTS, MINT, "Intercepting to mroutine %d, MSPSR = 0x%lx.", imm, mspsr);

            return Menter64::execute(xc, traceData);
        }
    }
}
