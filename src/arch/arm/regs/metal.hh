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
        Bitfield<59> em; // exc intercept mask
        Bitfield<58> pd; // privilege check disable
        Bitfield<57> id; // temporary interrupt disable
        Bitfield<7,0> lv; // Metal nesting level
    EndBitUnion(MSR_t)

    BitUnion8(MTPField)
        Bitfield<1, 0> ap;
        Bitfield<2> xn;
        Bitfield<3> rn;
    EndBitUnion(MTPField)

    enum : RegIndex
    {
        /* All the unique register indices. */
        MO0 = 0,
        MO1,
        MO2,
        MO3,
        MO4,
        MO5,
        MR0,
        MR1,
        MR2,
        MR3,
        MI0,
        MI1,
        MI2,
        MI3,
        MI4,
        MI5,
        NumGenRegs,
        // inst intercept alias
        MIR0 = MI0,
        MIR1 = MI1,
        MIR2 = MI2,
        // exc intercept alias
        MER0 = MI0,
        MER1 = MI1,
        MER2 = MI2,
        MSPSR = MI3,
        // Metal Link Register Alias
        MLR = MI5,

        MSR = NumGenRegs, // Metal Status Register
        MBR, // Metal Base Register
        MIB, // Metal Instruction Base Register
        MEB, // Metal Exception Base Register
        MTP, // Metal TLB Permissions Register
        MG5,
        MG6,
        MG7,
        MG8,
        MG9,
        MG10,
        MG11,
        MG12,
        MG13,
        MG14,
        MG15,
        NumRegs,
        NumMiscRegs = NumRegs - NumGenRegs,
    };
    static_assert(NumRegs == 32);
    static_assert(NumRegs <= (1 << 5));
    static constexpr size_t NumWindow = 64;
    static constexpr size_t WindowSize = NumGenRegs;
    static constexpr size_t WindowOverlap = 6;
    static constexpr size_t TotalGRegs = WindowOverlap + NumWindow * (WindowSize - WindowOverlap);

    const char * const regNames[] = {
        "mo0",
        "mo1",
        "mo2",
        "mo3",
        "mo4",
        "mo5",
        "mr0",
        "mr1",
        "mr2",
        "mr3",
        "mi0",
        "mi1",
        "mi2",
        "mi3",
        "mi4",
        "mi5",
        "msr",
        "mbr",
        "mib",
        "meb",
        "mg4",
        "mg5",
        "mg6",
        "mg7",
        "mg8",
        "mg9",
        "mg10",
        "mg11",
        "mg12",
        "mg13",
        "mg14",
        "mg15",
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

    static inline MTPField getMTPField(RegVal mtp, unsigned int idx)
    {
        assert(idx < 16);

        mtp = mtp >> (idx * 4);
        mtp = mtp & 0b1111;

        return static_cast<MTPField>(mtp);
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

    static inline bool isExcInterceptMasked(MSR_t msr)
    {
        return static_cast<bool>(msr.em);
    }

    static inline bool isInterruptDisabled(MSR_t msr)
    {
        return static_cast<bool>(msr.id);
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
