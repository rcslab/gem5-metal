#ifndef __ARCH_ARM_REGS_METAL_MISC_HH__
#define __ARCH_ARM_REGS_METAL_MISC_HH__


#include "arch/arm/metal.hh"
#include "cpu/metal_int_state.hh"
#include "debug/MetalRegs.hh"
#include "cpu/reg_class.hh"

namespace gem5
{

class ThreadContext;

namespace ArmISA
{
namespace metal
{
namespace reg
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
        // Bitfield<2> pd; // privileged instruction disable
    EndBitUnion(MFLAGS_t)

    constexpr static size_t MMVA_ADDRSHIFT = 10;
    BitUnion64(MMVA_t)
        Bitfield<0> valid; // instruction intercept enable
        Bitfield<63, MMVA_ADDRSHIFT> addr;
    EndBitUnion(MMVA_t)

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
        NumGlobalRegs,

        // aliases
        MSR = 0, // Metal Status Register (RO)
        MBR, // Metal Base Register
        MIB, // Metal Instruction Base Register
        MEB, // Metal Exception Base Register
        MTP, // Metal TLB Permissions Register
        MAR, // Metal Access Register
        MSTK, // Metal Stack Register
        MFLAGS, // Metal Flags Register
        MMPA, // MRAM physical memory map register (RO), 4k aligned 
        MMSZ, // MRAM size register (RO), must be a multiple of 4K.
        MMVA, // MRAM virtual memory map register
        NumMiscRegs
    };
    static_assert(NumGlobalRegs == 32);
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
        "mmpa",
        "mmsz",
        "mmva"
    };
    static_assert((sizeof(miscRegNames) / sizeof(miscRegNames[0])) == NumMiscRegs);

    const char * const globalRegNames[] = {
        "mg0",
        "mg1",
        "mg2",
        "mg3",
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
    static_assert((sizeof(globalRegNames) / sizeof(globalRegNames[0])) == NumGlobalRegs);

    // init regs can be accessed by WMCR/RMCR without being in metal mode when 
    // metal mode is not initialized yet
    static inline bool isInitReg(RegIndex idx)
    {
        return idx == MBR || idx == MSTK || idx == MMPA || idx == MMVA || idx == MMSZ;
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

    static inline bool getReadPerm(RegVal mar, RegIndex idx)
    {
        assert(idx < 32);

        return !((mar >> (2 * idx)) & 0x1);
    }

    static inline bool getWritePerm(RegVal mar, RegIndex idx)
    {
        assert(idx < 32);

        return !((mar >> (2 * idx + 1)) & 0x1);
    }

    static inline bool canAccessGlobalReg(RegIndex idx, RegVal mar, bool write)
    {
        return write ? getWritePerm(mar, idx) : getReadPerm(mar, idx);
    }

    static inline MSR_t getMSRFromState(const gem5::metal::InternalState & mist)
    {
        MSR_t msr = 0;
        const gem5::metal::InternalFlags flags = mist.getFlags();
        msr.lv = mist.getLevel();
        msr.id = flags.isSet(gem5::metal::FLAG_INST_INTERCEPT_MASK_TEMP);
        msr.im = flags.isSet(gem5::metal::FLAG_INTERRUPT_MASK_TEMP);
        msr.em = flags.isSet(gem5::metal::FLAG_EXC_INTERCEPT_MASK_TEMP);
        msr.init = flags.isSet(metal::METAL_FLAG_INIT);
        return msr;
    }

    static inline bool canAccessMiscReg(RegIndex idx, const gem5::metal::InternalState & state, bool write)
    {
        bool allowAccess = false;
        gem5::metal::InternalFlags flags = state.getFlags();
        if (!(flags & METAL_FLAG_INIT) && isInitReg(idx)) {
            // allow Metal initialization
            allowAccess = true;
        } else {
            allowAccess = state.getLevel() > 0;
        }

        return allowAccess;
    }
} // namespace reg
} // namespace metal

class MetalMiscRegClassOps : public RegClassOps {};
inline constexpr MetalMiscRegClassOps metalMiscRegClassOps;
inline constexpr RegClass metalMiscRegClass =
    RegClass(MetalMiscRegClass, MetalMiscRegClassName, metal::reg::NumMiscRegs, debug::MetalRegs).
    ops(metalMiscRegClassOps);

class MetalGlobalRegClassOps : public RegClassOps {};
inline constexpr MetalGlobalRegClassOps metalGlobalRegClassOps;
inline constexpr RegClass metalGlobalRegClass =
    RegClass(MetalGlobalRegClass, MetalGlobalRegClassName, metal::reg::NumGlobalRegs, debug::MetalRegs).
    ops(metalGlobalRegClassOps);
} // namespace ARMISA
} // namespace gem5

#endif
