#pragma once

#include "arch/arm/types.hh"
#include "base/bitunion.hh"
#include "base/intmath.hh"

namespace gem5 {
namespace ArmISA {
namespace metal {
        enum : uint64_t {
            METAL_FLAG_INIT = 0x1  
        };

        // mroutine stuff
        static constexpr size_t MroutineTableMaxEntryNum = 1 << 8;
        static constexpr uint64_t MroutineTableEntryAddrShift = 4;
        BitUnion64(MroutineTableEntry)
            Bitfield<63, MroutineTableEntryAddrShift> unshiftedAddr;
            Bitfield<MroutineTableEntryAddrShift - 1, 1> _unused;
            Bitfield<0> valid;
        EndBitUnion(MroutineTableEntry)
        static_assert(sizeof(MroutineTableEntry) == sizeof(uint64_t) && isPowerOf2(sizeof(MroutineTableEntry)));
        static constexpr size_t MroutineTableLoadSize = 8 * sizeof(MroutineTableEntry);
        static constexpr size_t MroutineTableTotalSize = MroutineTableMaxEntryNum * sizeof(MroutineTableEntry);

        // Instruction intercept
        BitUnion32(InstInterceptCtrl)
            Bitfield<7, 0> mroutine;
            Bitfield<31> valid;
        EndBitUnion(InstInterceptCtrl)
        struct InstInterceptTableEntry {
            MachInst inst;
            MachInst opMask;
            InstInterceptCtrl ctrl;
            MachInst mask0;
            MachInst mask1;
            MachInst mask2;
        };
        static_assert(sizeof(InstInterceptTableEntry) == sizeof(uint32_t) * 6);

        static constexpr size_t InstInterceptTableMaxEntryNum = 64;
        static constexpr size_t InstInterceptTableLoadSize = 2 * sizeof(InstInterceptTableEntry);
        static constexpr size_t InstInterceptTableTotalSize = InstInterceptTableMaxEntryNum * sizeof(InstInterceptTableEntry);
        static_assert((InstInterceptTableTotalSize % InstInterceptTableLoadSize) == 0 && (InstInterceptTableLoadSize % sizeof(InstInterceptTableEntry)) == 0);

        // Exception intercept
        BitUnion32(ExcInterceptCtrl)
            Bitfield<7, 0> mroutine;
            Bitfield<9, 8> mode;
            Bitfield<10> im;
            Bitfield<31> valid;
        EndBitUnion(ExcInterceptCtrl)

        struct ExcInterceptTableEntry {
            uint32_t excBits;
            uint32_t excMask;
            ExcInterceptCtrl ctrl;
        };
        static_assert(sizeof(ExcInterceptTableEntry) == sizeof(uint32_t) * 3);
        static constexpr size_t ExcInterceptTableMaxEntryNum = 64;
        static constexpr size_t ExcInterceptTableLoadSize = 4 * sizeof(ExcInterceptTableEntry);
        static constexpr size_t ExcInterceptTableTotalSize = ExcInterceptTableMaxEntryNum * sizeof(ExcInterceptTableEntry);
        static_assert((ExcInterceptTableTotalSize % ExcInterceptTableLoadSize) == 0 && (ExcInterceptTableLoadSize % sizeof(ExcInterceptTableEntry)) == 0);
}
}
}