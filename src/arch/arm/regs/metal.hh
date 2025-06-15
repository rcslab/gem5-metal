#ifndef __ARCH_ARM_REGS_METAL_HH__
#define __ARCH_ARM_REGS_METAL_HH__

#include "debug/MetalRegs.hh"
#include "cpu/reg_class.hh"
#include "metal_misc.hh"

namespace gem5
{

namespace ArmISA
{

namespace metal_reg
{
    enum : RegIndex
    {
        /* Each window is 32 registers
         *
         *
         *   TOP of RegStack (each level grows downward)
         *   |-------------|     |----------------|
         *   | Input  lv.n |     | N/A            |
         *   | Local  lv.n |     | N/A            |
         *   | Output lv.n |     | Input   lv.n+1 |
         *   |-------------|     | Local   lv.n+1 |
         *                       | Output  lv.n+1 |
         *                       |----------------|
         *
         * Window shift is 22 regs (Input + Local)
         * Each level requires 22 new regs
         * Total regs = Window Size + (Max Level - 1) * Window Shift
         *
        */
        MaxMetalLevel = 7,

        /* aliases for mregs in a certain window */
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

        ML0,
        ML1,
        ML2,
        ML3,
        ML4,
        ML5,
        ML6,
        ML7,
        ML8,
        ML9,
        ML10,
        ML11,

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
        WindowSize,
        WindowShift = MI9 - MO9,

        // aliases for other features
        MIR0 = ML0,
        MIR1 = ML1,
        MIR2 = ML2,

        MER0 = ML0,
        MER1 = ML1,

        MSPSR = ML9,
        MSFLAGS = ML10,
        MLR = ML11
    };
    static_assert(WindowSize == 32);
    static_assert(WindowSize <= (1 << 5));
    static_assert(MaxMetalLevel > 0);
    static constexpr size_t NumGRegs = WindowSize + MaxMetalLevel * (WindowShift);

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
        "mspsr",
        "msflags",
        "mlr",

        "mi0",
        "mi1",
        "mi2",
        "mi3",
        "mi4",
        "mi5",
        "mi6",
        "mi7",
        "mi8",
        "mi9",
    };
    static_assert((sizeof(regNames) / sizeof(regNames[0])) == WindowSize);
} // namespace metal_reg

inline constexpr RegClass flatMetalRegClass =
    RegClass(MetalRegClass, MetalRegClassName, metal_reg::NumGRegs, debug::MetalRegs);

class MetalRegClassOps : public RegClassOps
{
    RegId flatten(const BaseISA &isa, const RegId &id) const override;
    RegId flatten(ExecContext * xc, const RegId &id) const override;
    static RegId flattenWithStates(metal_reg::MSR_t msr, const RegId &id);
};

inline constexpr MetalRegClassOps metalRegClassOps;

inline constexpr RegClass metalRegClass =
    RegClass(MetalRegClass, MetalRegClassName, metal_reg::NumGRegs, debug::MetalRegs).
    ops(metalRegClassOps).needsFlattening();

} // namespace ARMISA
} // namespace gem5

#endif
