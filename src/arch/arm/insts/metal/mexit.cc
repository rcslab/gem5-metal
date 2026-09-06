#include "arch/arm/insts/metal/mexit.hh"
#include "arch/arm/regs/metal.hh"
#include "cpu/metal_int_state.hh"

namespace gem5 { namespace ArmISA {
namespace metal{ namespace inst {
        Mexit64::Mexit64(ExtMachInst _machInst, uint _imm) : MetalImmOp("mexit", _machInst, IntAluOp, _imm)
        {
            this->flags[IsControl] = true;
            this->flags[IsIndirectControl] = true;
            this->flags[IsUncondControl] = true;
            this->flags[IsReturn] = true;
            this->flags[IsPreExecOperandUpdate] = true;
        }

        Fault Mexit64::preExec(ExecContext *xc, trace::InstRecord *traceData)
        {
            auto state = xc->getMetalState();

            if (state.getLevel() == 0) {
                return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
            }

            // manually flatten registers to pre metal state change
            setSrcRegIdx(_numSrcRegs++, metalRegClass[reg::MLR].flatten(xc));

            const MexitFlags flags = static_cast<MexitFlags>(imm);
            if (flags.rfi) {
                //
                // "lightweight" exception: only restore COND flags
                //
                setSrcRegIdx(_numSrcRegs++, metalRegClass[reg::MSPSR].flatten(xc));
                // setSrcRegIdx(_numSrcRegs++, metalRegClass[metal_reg::MSFLAGS].flatten(xc));

                setDestRegIdx(_numDestRegs++, ccRegClass[cc_reg::Nz].flatten(xc));
                setDestRegIdx(_numDestRegs++, ccRegClass[cc_reg::C].flatten(xc));
                setDestRegIdx(_numDestRegs++, ccRegClass[cc_reg::V].flatten(xc));
                _numTypedDestRegs[ccRegClass.type()] += 3;
            }

            if (flags.id) {
                state.setFlags(gem5::metal::FLAG_INTERRUPT_MASK_TEMP);
            }

            if (flags.eim) {
                state.setFlags(gem5::metal::FLAG_EXC_INTERCEPT_MASK_TEMP);
            }

            if (flags.iim) {
                state.setFlags(gem5::metal::FLAG_INST_INTERCEPT_MASK_TEMP);
            }

            state.setLevel(state.getLevel() - 1);
            xc->setMetalState(state);

            return NoFault;
        }

        Fault Mexit64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            const gem5::metal::InternalState state = xc->getMetalState();
            assert(state.getLevel() <= reg::MaxMetalLevel);

            const RegVal ret = xc->getRegOperand(this, 0);
            const MexitFlags flags = static_cast<MexitFlags>(imm);

            // set new PC
            const Addr target_addr = purifyTaggedAddr(ret, xc->tcBase(), currEL(xc->tcBase()), true);
            PCState pcState;
            set(pcState, xc->pcState());
            pcState.instNPC(target_addr);
            xc->pcState(pcState);

            CPSR mspsr = 0;
            if (flags.rfi) {
                mspsr = xc->getRegOperand(this, 1);

                // restore NzCV
                xc->setRegOperand(this, 0, mspsr.nz);
                xc->setRegOperand(this, 1, mspsr.c);
                xc->setRegOperand(this, 2, mspsr.v);

                // restore MFLAGS
                // metal_reg::MFLAGS_t mflags = xc->getRegOperand(this, 2);
                // tc->setMetalMiscReg(metal_reg::MFLAGS, msflags);
            }

            METAL_DBGPRINT(INSTS, MEXIT, "Exiting Metal mode (Lv.%d): MLR = 0x%lx, flags = [id = %d, rfi = %d (MSPSR = 0x%lx), iim = %d, eim = %d]\n",
                                                state.getLevel(), ret, flags.id, flags.rfi, mspsr, flags.iim, flags.eim);

            return NoFault;
        }
}}
}}
