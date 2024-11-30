#ifndef __ARCH_ARM_REGS_METAL_HH__
#define __ARCH_ARM_REGS_METAL_HH__

#include "arch/arm/types.hh"
#include "base/logging.hh"
#include "debug/MetalRegs.hh"
#include "cpu/reg_class.hh"
#include "sim/core.hh"

namespace gem5
{

namespace ArmISA
{

namespace metal_reg
{
    enum : RegIndex
    {
        /* All the unique register indices. */
        MO0 = 0,
        MO1,
        MO2,
        MO3,
        MO4,
        MO5,
        MO6,
        MO7,
        MO8,
        MO9,
        NumIORegs,

        MR0 = NumIORegs,
        MR1,
        MR2,
        MR3,
        MR4,
        MR5,
        MR6,
        MR7,
        MR8,
        MR9,
        MR10,
        MR11,

        MI0,
        MI1,
        MI2,
        MI3,
        MI4,
        MI5,
        MI6,
        MI7,
        MI8,
        MI9,
        NumRegs,

        // aliases 
        MIR0 = MI0,
        MIR1 = MI1,
        MIR2 = MI2,

        MER0 = MI0,
        MER1 = MI1,

        MSPSR = MI8,
        MLR = MI9
    };
    static_assert(NumRegs == 32);
    static_assert(NumRegs <= (1 << 5));
    static constexpr size_t NumWindow = 9;
    static_assert(NumWindow > 0);
    static constexpr size_t TotalGRegs = NumRegs + (NumWindow - 1) * (NumRegs - NumIORegs);

    const char * const regNames[] = {
        "mo0",
        "mo1",
        "mo2",
        "mo3",
        "mo4",
        "mo5",
        "mo6",
        "mo7",
        "mo8",
        "mo9",

        "mr0",
        "mr1",
        "mr2",
        "mr3",
        "mr4",
        "mr5",
        "mr6",
        "mr7",
        "mr8",
        "mr9",
        "mr10",
        "mr11",

        "mi0",
        "mi1",
        "mi2",
        "mi3",
        "mi4",
        "mi5",
        "mi6",
        "mi7",
        "mspsr",
        "mlr",
    };
    static_assert((sizeof(regNames) / sizeof(regNames[0])) == NumRegs);
} // namespace metal_reg

class MetalRegClassOps : public RegClassOps {};

inline constexpr MetalRegClassOps metalRegClassOps;

inline constexpr RegClass metalRegClass =
    RegClass(MetalRegClass, MetalRegClassName, metal_reg::NumRegs, debug::MetalRegs).
    ops(metalRegClassOps);

} // namespace ARMISA
} // namespace gem5

#endif
