#ifndef __ARCH_ARM_REGS_METAL_MISC_HH__
#define __ARCH_ARM_REGS_METAL_MISC_HH__


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
        Bitfield<63> init; // whether metal is initialized
        Bitfield<62> im; // instruction intercept mask
        Bitfield<61> em; // exc intercept mask
        Bitfield<60> id; // temporary interrupt disable
        Bitfield<7,0> lv; // Metal nesting level
    EndBitUnion(MSR_t)

    BitUnion64(MFLAGS_t)
        Bitfield<0> ii; // instruction intercept enable
        Bitfield<1> ei; // exc intercept enable
        Bitfield<2> pd; // privileged instruction disable 
    EndBitUnion(MFLAGS_t)

    BitUnion8(MTPField)
        Bitfield<0> read;
        Bitfield<1> write;
        Bitfield<2> execute;
    EndBitUnion(MTPField)

    enum : RegIndex
    {
        MG0,
        MG1,
        MG2,
        MG3,
        MG4,
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
        MG16,
        MG17,
        MG18,
        MG19,
        MG20,
        MG21,
        MG22,
        MG23,
        MG24,
        MG25,
        MG26,
        MG27,
        MG28,
        MG29,
        MG30,
        MG31,
        NumMiscRegs,

        // aliases
        MSR = MG0, // Metal Status Register (RO)
        MBR = MG1, // Metal Base Register
        MIB = MG2, // Metal Instruction Base Register
        MEB = MG3, // Metal Exception Base Register
        MTP = MG4, // Metal TLB Permissions Register
        MAR = MG5, // Metal Access Register
        MSTK = MG6, // Metal Stack Register
        MFLAGS = MG7 // Metal Flags Register 
    };
    static_assert(NumMiscRegs == 32);
    static_assert(NumMiscRegs <= (1 << 5));

    const char * const miscRegNames[] = {
        "msr",
        "mbr",
        "mib",
        "meb",
        "mtp",
        "mar",
        "mstk",
        "mflags",
        "mg8",
        "mg9",
        "mg10",
        "mg11",
        "mg12",
        "mg13",
        "mg14",
        "mg15",
        "mg16",
        "mg17",
        "mg18",
        "mg19",
        "mg20",
        "mg21",
        "mg22",
        "mg23",
        "mg24",
        "mg25",
        "mg26",
        "mg27",
        "mg28",
        "mg29",
        "mg30",
        "mg31"
    };
    static_assert((sizeof(miscRegNames) / sizeof(miscRegNames[0])) == NumMiscRegs);

    static inline unsigned long getMetalLevel(MSR_t msr)
    {
        return msr.lv;
    }

    static inline bool isInMetalMode(MSR_t msr)
    {
        return getMetalLevel(msr) > 0;
    }

    static inline bool isInitReg(RegIndex idx)
    {
        return idx == MBR || idx == MSTK;
    }

    static inline bool isSetInit(RegIndex idx)
    {
        return idx == MBR;
    }

    static inline MTPField getMTPField(RegVal mtp, unsigned int idx)
    {
        assert(idx < 16);

        mtp = mtp >> (idx * 4);
        mtp = mtp & 0b1111;

        return static_cast<MTPField>(mtp);
    }

    static inline bool getReadPerm(RegVal mar, unsigned int idx)
    {
        assert(idx < 32);

        return !((mar >> (2 * idx)) & 0x1);
    }

    static inline bool getWritePerm(RegVal mar, unsigned int idx)
    {
        assert(idx < 32);

        return !((mar >> (2 * idx + 1)) & 0x1);
    }

    static inline bool isPrivInstsEnabled(MFLAGS_t mflags)
    {
        return !static_cast<bool>(mflags.pd);
    }

    static inline bool isInstInterceptEnabled(MFLAGS_t mflags)
    {
        return static_cast<bool>(mflags.ii);
    }

    static inline bool isExcInterceptEnabled(MFLAGS_t mflags)
    {
        return static_cast<bool>(mflags.ei);
    }

    static inline bool isInstInterceptMasked(MSR_t msr)
    {
        return static_cast<bool>(msr.im);
    }

    static inline bool isExcInterceptMasked(MSR_t msr)
    {
        return static_cast<bool>(msr.em);
    }

    static inline bool isInterruptDisabled(MSR_t msr)
    {
        return static_cast<bool>(msr.id);
    }

} // namespace metal_reg

class MetalMiscRegClassOps : public RegClassOps {};

inline constexpr MetalMiscRegClassOps metalMiscRegClassOps;

inline constexpr RegClass metalMiscRegClass =
    RegClass(MetalMiscRegClass, MetalMiscRegClassName, metal_reg::NumMiscRegs, debug::MetalRegs).
    ops(metalMiscRegClassOps);

} // namespace ARMISA
} // namespace gem5

#endif