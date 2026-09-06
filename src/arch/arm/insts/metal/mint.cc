#include "arch/arm/insts/metal/mint.hh"
#include "arch/arm/insts/metal/menter.hh"
#include "arch/arm/regs/cc.hh"
#include "arch/arm/regs/metal.hh"
#include "base/types.hh"
#include "cpu/static_inst.hh"
#include "enums/StaticInstFlags.hh"

namespace gem5 { namespace ArmISA {
namespace metal { namespace inst {
    Mint64::Mint64(ExtMachInst _machInst, uint _imm, RegVal mir0, RegVal mir1, RegVal mir2) :
    Menter64("mint", _machInst, _imm), mir0(mir0), mir1(mir1), mir2(mir2)
    {
        this->flags[IsControl] = true;
        this->flags[IsIndirectControl] = true;
        this->flags[IsUncondControl] = true;
        this->flags[IsCall] = true;

        setDestRegIdx(_numDestRegs++, metalRegClass[reg::MSPSR]); // idx = 1
        setDestRegIdx(_numDestRegs++, metalRegClass[reg::MIR0]); // idx = 2
        setDestRegIdx(_numDestRegs++, metalRegClass[reg::MIR1]); // idx = 3
        setDestRegIdx(_numDestRegs++, metalRegClass[reg::MIR2]); // idx = 4
        _numTypedDestRegs[metalRegClass.type()] += 4;

        // setDestRegIdx(_numDestRegs++, metalRegClass[metal_reg::MSFLAGS].flatten(xc));
        // _numTypedDestRegs[metalRegClass.type()]++;

        setSrcRegIdx(_numSrcRegs++, ccRegClass[cc_reg::Nz]); // idx = 0
        setSrcRegIdx(_numSrcRegs++, ccRegClass[cc_reg::C]); // idx = 1
        setSrcRegIdx(_numSrcRegs++, ccRegClass[cc_reg::V]); // idx = 2
    }

    Fault Mint64::preExec(ExecContext *xc, trace::InstRecord *traceData)
    {
        return Menter64::preExec(xc, traceData);
    }

    std::unique_ptr<PCStateBase> Mint64::buildRetPC(const PCStateBase &cur_pc, const PCStateBase &call_pc) const
    {
        PCStateBase *ret_pc = call_pc.clone();
        ret_pc->as<PCState>().uReset();
        return std::unique_ptr<PCStateBase>{ret_pc};
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

        // set the three masks
        xc->setRegOperand(this, 2, mir0);
        xc->setRegOperand(this, 3, mir1);
        xc->setRegOperand(this, 4, mir2);

        METAL_DBGPRINT(INSTS, MINT, "Intercepting to mroutine %d, MSPSR = 0x%lx, MIRs = [0x%lx, 0x%lx, 0x%lx].\n", imm, mspsr, mir0, mir1, mir2);

        return Menter64::execute(xc, traceData, xc->pcState().instAddr());
    }

}}
}}
