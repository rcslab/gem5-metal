#include <cassert>

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
    BitUnion64(MSR_t)
        Bitfield<63> init; // Metal initialization flag
        Bitfield<62> ic; // instruction intercept enable
        Bitfield<61,0> lv; // Metal nesting level
    EndBitUnion(MSR_t)

    enum : RegIndex
    {
        /* All the unique register indices. */
        MR0 = 0,
        MR1 = 1,
        MR2 = 2,
        MR3 = 3,
        MR4 = 4,
        MR5 = 5,
        MR6 = 6,
        MR7 = 7,
        MR8 = 8,
        MR9 = 9,
        MR10 = 10,
        MR11 = 11,
        MR12 = 12,
        MR13 = 13,
        MR14 = 14,
        MR15 = 15,
        NumGenRegs = MR15 + 1,

        MSR = 16, // Metal Status Register
        MBR = 17, // Metal Base Register
        MLR = 18, // Metal Link Register
        MIB = 19, // Metal Instruction Base Register

        NumRegs = MIB + 1
    };

    const char * const regNames[] = {
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
        "mr12",
        "mr13",
        "mr14",
        "mr15",
        "msr",
        "mbr",
        "mlr",
        "mib"
    };

    static inline unsigned long getMetalLevel(ThreadContext * tc)
    {
        MSR_t msr = tc->readMetalRegNoEffect(metal_reg::MSR);
        return msr.lv;
    }

    static inline bool isInMetalMode(ThreadContext * tc)
    {
        return getMetalLevel(tc) > 0;
    }

    static inline bool isMetalInitialized(ThreadContext *tc)
    {
        MSR_t msr = tc->readMetalRegNoEffect(metal_reg::MSR);
        return (bool)msr.init;
    }

    static inline bool canWriteMetalReg(ThreadContext *tc, RegIndex mreg)
    {
        if (mreg >= NumRegs) {
            // access beyond the number of parameters
            return false;
        }

        if (isInMetalMode(tc)) {
            // allow access in full metal mode
            return true;
        }

        if (mreg == MBR && !isMetalInitialized(tc)) {
            // allow writing to Metal Base Register outside of Metal mode to initialize Metal
            return true;    
        }

        return false;
    }

    static inline bool canReadMetalReg(ThreadContext *tc, RegIndex mreg)
    {
        if (mreg >= NumRegs) {
            // access beyond the number of parameters
            return false;
        }

        return isInMetalMode(tc);
    }

} // namespace metal_reg

class MetalRegClassOps : public RegClassOps {};

inline constexpr MetalRegClassOps metalRegClassOps;

inline constexpr RegClass metalRegClass =
    RegClass(MetalRegClass, MetalRegClassName, metal_reg::NumRegs, debug::MetalRegs).
    ops(metalRegClassOps);

} // namespace ARMISA
} // namespace gem5

#endif
