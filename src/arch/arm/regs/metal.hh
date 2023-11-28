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
        Bitfield<62> ii; // instruction intercept enable
        Bitfield<61> im; // instruction intercept mask
        Bitfield<60> ei; // exc intercept enable
        Bitfield<59> pd; // privilege check disable
        Bitfield<7,0> lv; // Metal nesting level
    EndBitUnion(MSR_t)

    enum : RegIndex
    {
        /* All the unique register indices. */
        MR0 = 0,
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
        MR12,
        MR13,
        MR14,
        MR15,
        MR16,
        MR17,
        MR18,
        MR19,
        MR20,
        MR21,
        MR22,
        MR23,
        MIR0 = MR16,
        MIR1 = MR17,
        MIR2 = MR18,
        MER0 = MR16,
        MER1 = MR17,
        MER2 = MR18,
        MSPSR = MR22,
        MLR = MR23,
        NumGenRegs,

        MSR = NumGenRegs, // Metal Status Register
        MBR, // Metal Base Register
        MIB, // Metal Instruction Base Register
        MEB, // Metal Exception Base Register
        MTP, // Metal TLB Permissions Register
        MG5,
        MG6,
        MG7,
        NumRegs,
        NumMiscRegs = NumRegs - NumGenRegs,
    };
    static_assert(NumRegs <= (1 << 5));
    static constexpr size_t NumWindow = 64;
    static constexpr size_t WindowSize = NumGenRegs;
    static constexpr size_t WindowOverlap = 8;
    static constexpr size_t TotalGRegs = WindowOverlap + NumWindow * (WindowSize - WindowOverlap);

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
        "mr16",
        "mr17",
        "mr18",
        "mr19",
        "mr20",
        "mr21",
        "mr22",
        "mlr",
        "msr",
        "mbr",
        "mib",
        "meb",
        "mg4",
        "mg5",
        "mg6",
        "mg7"
    };
    static_assert((sizeof(regNames) / sizeof(regNames[0])) == NumRegs);

    static inline bool isGeneralReg(RegIndex idx)
    {
        return idx < NumGenRegs;
    }

    static inline unsigned long getMetalLevel(MSR_t msr)
    {
        return msr.lv;
    }

    static inline bool isInMetalMode(MSR_t msr)
    {
        return getMetalLevel(msr) > 0;
    }

    static inline bool isMetalInitialized(MSR_t msr)
    {
        return static_cast<bool>(msr.init);
    }

    static inline bool isInstInterceptEnabled(MSR_t msr)
    {
        return static_cast<bool>(msr.ii);
    }

    static inline bool isInstInterceptMasked(MSR_t msr)
    {
        return static_cast<bool>(msr.im);
    }

    static inline bool isExcInterceptEnabled(MSR_t msr)
    {
        return static_cast<bool>(msr.ei);
    }

    static inline bool isPrivilegeCheckDisabled(MSR_t msr)
    {
        return static_cast<bool>(msr.pd);
    }

    static inline bool canWriteMetalReg(MSR_t msr, RegIndex mreg)
    {
        if (isPrivilegeCheckDisabled(msr)) {
            return true;
        }

        if (mreg >= NumRegs) {
            // access beyond the number of parameters
            return false;
        }

        // allow non metal mode to access all metal general regs because of windowing
        if (mreg < NumGenRegs) {
            return true;
        }

        // the rest of metal regs can only be accessed in metal mode
        return isInMetalMode(msr);
    }

    static inline bool canReadMetalReg(MSR_t msr, RegIndex mreg)
    {
        if (isPrivilegeCheckDisabled(msr)) {
            return true;
        }

        if (mreg >= NumRegs) {
            // access beyond the number of parameters
            return false;
        }

        // allow non metal mode to access all metal general regs because of windowing
        if (mreg < NumGenRegs) {
            return true;
        }


        // the rest of metal regs can only be accessed in metal mode
        return isInMetalMode(msr);
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
