#include "arch/arm/insts/metal/mint.hh"
#include "arch/generic/memhelpers.hh"

namespace gem5 {
    namespace ArmISA {
        Mint64::Mint64(ExtMachInst _machInst, uint8_t _imm) : Menter64(_machInst, _imm)
        {
            setDestRegIdx(_numDestRegs++, metalRegClass[metal_reg::MSPSR]);
            _numTypedDestRegs[metalRegClass.type()]++;

            setSrcRegIdx(_numSrcRegs++, ccRegClass[cc_reg::Nz]);
            setSrcRegIdx(_numSrcRegs++, ccRegClass[cc_reg::C]);
            setSrcRegIdx(_numSrcRegs++, ccRegClass[cc_reg::V]);

            this->flags[IsInteger] = true;
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

            METAL_DBGPRINT(INSTS, MINT, "Intercepting to mroutine %d, MSPSR = 0x%lx.", imm, mspsr);

            Menter64::execute(xc, traceData);
            
            return NoFault;
        }
    }
}