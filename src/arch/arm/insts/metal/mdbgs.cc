#include "arch/arm/insts/metal/mdbgs.hh"
#include "arch/arm/regs/metal.hh"
#include "base/debug.hh"
#include "base/types.hh"
#include "cpu/metal_int_state.hh"
#include <sstream>

namespace gem5 { namespace ArmISA {
namespace metal{ namespace inst {
        Mdbgs64::Mdbgs64(ExtMachInst _machInst, uint _imm) : MetalImmOp("mdbgs", _machInst, IntAluOp, _imm)
        {
            this->flags[IsSerializeAfter] = true;
            this->flags[IsNonSpeculative] = true;
        }

        Fault Mdbgs64::execute(ExecContext *xc, trace::InstRecord *traceData) const
        {
            std::stringstream ss;

            for(unsigned int i = 0; i < std::size(FLAGS); i++) {
                if (bits(imm, i) ) {
                    if (i > 0) {
                        ss << ", ";
                    }

                    ss << FLAGS[i];
                    setDebugFlag(FLAGS[i].data());
                } else {
                    clearDebugFlag(FLAGS[i].data());
                }
            }

            METAL_DBGPRINT(INSTS, MDBGS64, "setting debug flags to 0x%lx [%s].\n", imm, ss.str());

            return NoFault;
        }
}}
}}
