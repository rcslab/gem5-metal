import argparse
import os

import m5
from m5.objects import *
from m5.options import *
from m5.util import addToPath
import pdb;
from m5.objects.Ide import *

from gem5.simulate.exit_event import ExitEvent

m5.util.addToPath("../..")

import devices
from common import (
    MemConfig,
    ObjectList,
    SysPaths,
)
from common.cores.arm import (
    HPI,
    O3_ARM_v7a,
)

class L1I(Cache):
    tag_latency = 1
    data_latency = 1
    response_latency = 1
    mshrs = 2
    tgts_per_mshr = 8
    size = "32kB"
    assoc = 2
    is_read_only = True
    # Writeback clean lines as well
    writeback_clean = True


class L1D(Cache):
    tag_latency = 2
    data_latency = 2
    response_latency = 2
    mshrs = 6
    tgts_per_mshr = 8
    size = "32kB"
    assoc = 2
    write_buffers = 16
    # Consider the L2 a victim cache also for clean lines
    writeback_clean = True

# L2 Cache
class L2(Cache):
    tag_latency = 12
    data_latency = 12
    response_latency = 12
    mshrs = 16
    tgts_per_mshr = 8
    size = "1MB"
    assoc = 16
    write_buffers = 8
    clusivity = "mostly_excl"
    # Simple stride prefetcher
    prefetcher = StridePrefetcher(degree=8, latency=1, prefetch_on_access=True)
    tags = BaseSetAssoc()
    replacement_policy = RandomRP()


# Pre-defined CPU configurations. Each tuple must be ordered as : (cpu_class,
# l1_icache_class, l1_dcache_class, walk_cache_class, l2_Cache_class). Any of
# the cache class may be 'None' if the particular cache is not present.
cpu_types = {
    "atomic": (AtomicSimpleCPU, None, None, None),
    "timing": (TimingSimpleCPU, L1I, L1D, L2),
    "minor": (MinorCPU, devices.L1I, devices.L1D, devices.L2),
    "hpi": (HPI.HPI, HPI.HPI_ICache, HPI.HPI_DCache, HPI.HPI_L2),
    "o3": (
        O3_ARM_v7a.O3_ARM_v7a_3,
        O3_ARM_v7a.O3_ARM_v7a_ICache,
        O3_ARM_v7a.O3_ARM_v7a_DCache,
        O3_ARM_v7a.O3_ARM_v7aL2,
    ),
}

pmu_control_events = {
    "enable": ExitEvent.PERF_COUNTER_ENABLE,
    "disable": ExitEvent.PERF_COUNTER_DISABLE,
    "reset": ExitEvent.PERF_COUNTER_RESET,
}

pmu_interrupt_events = {
    "interrupt": ExitEvent.PERF_COUNTER_INTERRUPT,
}

pmu_stats_events = dict(**pmu_control_events, **pmu_interrupt_events)


class CowIdeDisk(IdeDisk):
    image = CowDiskImage(child=RawDiskImage(read_only=True), read_only=False)

    def childImage(self, ci):
        self.image.child.image_file = ci

def create(args):
    """Create and configure the syste   m object."""

    if args.readfile and not os.path.isfile(args.readfile):
        print(f"Error: Bootscript {args.readfile} does not exist")
        sys.exit(1)

    object_file = args.kernel if args.kernel else ""

    cpu_class = cpu_types[args.cpu][0]
    mem_mode = cpu_class.memory_mode()
    pci_devices = []
    # Only simulate caches when using a timing CPU (e.g., the HPI model)
    want_caches = True if mem_mode == "timing" else False

    platform = ObjectList.platform_list.get(args.machine_type)

    system = devices.SimpleSystem(
        want_caches,
        args.mem_size,
        platform=platform(),
        mem_mode=mem_mode,
        readfile=args.readfile,
    )

    MemConfig.config_mem(args, system)

    if args.semi_enable:
        system.semihosting = ArmSemihosting(
            stdin=args.semi_stdin,
            stdout=args.semi_stdout,
            stderr=args.semi_stderr,
            files_root_dir=args.semi_path,
            cmd_line=" ".join([object_file] + args.args),
        )

    system.realview.ethernet = IGbE_e1000(
            # pci_bus=0, pci_dev=0, pci_func=0, 
            InterruptLine=1, InterruptPin=1
    )
    pci_devices.append(system.realview.ethernet)
    system.realview.ide = IdeController(
            disks=[],
            # pci_func=1,
            # pci_dev=1,
            # pci_bus=1,
            InterruptLine=2, InterruptPin=2
    )

    if args.disk_image:
        disk = CowIdeDisk(driveID='device0')
        disk.childImage(args.disk_image)
        system.realview.ide.disks.append(disk)

    pci_devices.append(system.realview.ide)

    # Wire up the system's memory system
    system.connect()

    # Add CPU clusters to the system
    system.cpu_cluster = [
        devices.ArmCpuCluster(
            system,
            args.num_cores,
            args.cpu_freq,
            "1.0V",
            *cpu_types[args.cpu],
            tarmac_gen=args.tarmac_gen,
            tarmac_dest=args.tarmac_dest,
        )
    ]

    # Create a cache hierarchy for the cluster. We are assuming that
    # clusters have core-private L1 caches and an L2 that's shared
    # within the cluster.
    system.addCaches(want_caches, last_cache_level=2)

    # Setup gem5's minimal Linux boot loader.
    system.auto_reset_addr = True

    # Using GICv3
    if hasattr(system.realview.gic, "gicv4"):
        system.realview.gic.gicv4 = False

    system.highest_el_is_64 = True

    system.workload = ArmFsCastor()
    system.workload.object_file = object_file
    system.workload.dtb_addr = args.dtbaddr

    # pci devices
    for dev in pci_devices:
        system.realview.attachPciDevice(
            dev, system.iobus
        )
 
    if args.gdb:
        system.workload.wait_for_remote_gdb = True

    if args.dtb:
        system.workload.dtb_filename = args.dtb
    else:
        # No DTB specified: autogenerate DTB
        system.workload.dtb_filename = os.path.join(
            m5.options.outdir, "system.dtb"
        )
        system.generateDtb(system.workload.dtb_filename)

    if args.bootloader != "":
        system.realview.setupBootLoader(
            system, SysPaths.binary, args.bootloader
        )

    if args.with_pmu:
        enabled_pmu_events = {
            *args.pmu_dump_stats_on,
            *args.pmu_reset_stats_on,
        }
        exit_sim_on_control = bool(
            enabled_pmu_events & set(pmu_control_events.keys())
        )
        exit_sim_on_interrupt = bool(
            enabled_pmu_events & set(pmu_interrupt_events.keys())
        )
        for cluster in system.cpu_cluster:
            interrupt_numbers = [args.pmu_ppi_number] * len(cluster)
            cluster.addPMUs(
                interrupt_numbers,
                exit_sim_on_control=exit_sim_on_control,
                exit_sim_on_interrupt=exit_sim_on_interrupt,
            )

    if args.exit_on_uart_eot:
        for uart in system.realview.uart:
            uart.end_on_eot = True

    return system


def run(args):
    cptdir = m5.options.outdir
    if args.checkpoint:
        print(f"Checkpoint directory: {cptdir}")

    pmu_exit_msgs = tuple(evt.value for evt in pmu_stats_events.values())
    pmu_stats_dump_msgs = tuple(
        pmu_stats_events[evt].value for evt in set(args.pmu_dump_stats_on)
    )
    pmu_stats_reset_msgs = tuple(
        pmu_stats_events[evt].value for evt in set(args.pmu_reset_stats_on)
    )

    while True:
        event = m5.simulate()
        exit_msg = event.getCause()
        if exit_msg == ExitEvent.CHECKPOINT.value:
            print(f"Dropping checkpoint at tick {m5.curTick():d}")
            cpt_dir = os.path.join(m5.options.outdir, "cpt.%d" % m5.curTick())
            m5.checkpoint(os.path.join(cpt_dir))
            print("Checkpoint done.")
        elif exit_msg in pmu_exit_msgs:
            if exit_msg in pmu_stats_dump_msgs:
                print(
                    f"Dumping stats at tick {m5.curTick():d}, "
                    f"due to {exit_msg}"
                )
                m5.stats.dump()
            if exit_msg in pmu_stats_reset_msgs:
                print(
                    f"Resetting stats at tick {m5.curTick():d}, "
                    f"due to {exit_msg}"
                )
                m5.stats.reset()
        else:
            print(f"{exit_msg} ({event.getCode()}) @ {m5.curTick()}")
            break


def arm_ppi_arg(int_num: int) -> int:
    """Argparse argument parser for valid Arm PPI numbers."""
    # PPIs (1056 <= int_num <= 1119) are not yet supported by gem5
    int_num = int(int_num)
    if 16 <= int_num <= 31:
        return int_num
    raise ValueError(f"{int_num} is not a valid Arm PPI number")


def main():
    parser = argparse.ArgumentParser(epilog=__doc__)

    parser.add_argument(
        "--kernel", type=str, default=None, help="Binary to run"
    )
    parser.add_argument(
        "--disk-image", type=str, default=None, help="Disk to instantiate"
    )
    parser.add_argument(
        "--readfile",
        type=str,
        default="",
        help="File to return with the m5 readfile command",
    )
    parser.add_argument(
        "--cpu",
        type=str,
        choices=list(cpu_types.keys()),
        default="o3",
        help="CPU model to use",
    )
    parser.add_argument("--cpu-freq", type=str, default="1GHz")
    parser.add_argument(
        "--num-cores", type=int, default=1, help="Number of CPU cores"
    )
    parser.add_argument(
        "--machine-type",
        type=str,
        choices=ObjectList.platform_list.get_names(),
        default="VExpress_GEM5_V2",
        help="Hardware platform class",
    )
    parser.add_argument(
        "--mem-type",
        default="DDR3_1600_8x8",
        choices=ObjectList.mem_list.get_names(),
        help="type of memory to use",
    )
    parser.add_argument(
        "--mem-channels", type=int, default=1, help="number of memory channels"
    )
    parser.add_argument(
        "--mem-ranks",
        type=int,
        default=None,
        help="number of memory ranks per channel",
    )
    parser.add_argument(
        "--mem-size",
        action="store",
        type=str,
        default="2GB",
        help="Specify the physical memory size",
    )
    parser.add_argument("--checkpoint", action="store_true")
    parser.add_argument("--restore", type=str, default=None)
    parser.add_argument(
        "--tarmac-gen",
        action="store_true",
        help="Write a Tarmac trace.",
    )
    parser.add_argument(
        "--tarmac-dest",
        choices=TarmacDump.vals,
        default="stdoutput",
        help="Destination for the Tarmac trace output. [Default: stdoutput]",
    )
    parser.add_argument(
        "--with-pmu",
        action="store_true",
        help="Add a PMU to each core in the cluster.",
    )
    parser.add_argument(
        "--gdb",
        action="store_true",
        help="Wait for GDB connection.",
    )
    parser.add_argument(
        "--pmu-ppi-number",
        type=arm_ppi_arg,
        default=23,
        help="The number of the PPI to use to connect each PMU to its core. "
        "Must be an integer and a valid PPI number (16 <= int_num <= 31).",
    )
    parser.add_argument(
        "--pmu-dump-stats-on",
        type=str,
        default=[],
        action="append",
        choices=pmu_stats_events.keys(),
        help="Specify the PMU events on which to dump the gem5 stats. "
        "This option may be specified multiple times to enable multiple "
        "PMU events.",
    )
    parser.add_argument(
        "--pmu-reset-stats-on",
        type=str,
        default=[],
        action="append",
        choices=pmu_stats_events.keys(),
        help="Specify the PMU events on which to reset the gem5 stats. "
        "This option may be specified multiple times to enable multiple "
        "PMU events.",
    )
    parser.add_argument(
        "--exit-on-uart-eot",
        action="store_true",
        help="Exit simulation if any of the UARTs receive an EOT. Many "
        "workloads signal termination by sending an EOT character.",
    )
    parser.add_argument(
        "--dtb-gen",
        action="store_true",
        help="Doesn't run simulation, it generates a DTB only",
    )
    parser.add_argument(
        "--semi-enable", action="store_true", help="Enable semihosting support"
    )
    parser.add_argument(
        "--semi-stdin",
        type=str,
        default="stdin",
        help="Standard input for semihosting (default: gem5's stdin)",
    )
    parser.add_argument(
        "--semi-stdout",
        type=str,
        default="stdout",
        help="Standard output for semihosting (default: gem5's stdout)",
    )
    parser.add_argument(
        "--semi-stderr",
        type=str,
        default="stderr",
        help="Standard error for semihosting (default: gem5's stderr)",
    )
    parser.add_argument(
        "--semi-path",
        type=str,
        default="",
        help=("Search path for files to be loaded through Arm Semihosting"),
    )
    parser.add_argument(
        "args",
        default=[],
        nargs="*",
        help="Semihosting arguments to pass to benchmark",
    )
    parser.add_argument(
        "-P",
        "--param",
        action="append",
        default=[],
        help="Set a SimObject parameter relative to the root node. "
        "An extended Python multi range slicing syntax can be used "
        "for arrays. For example: "
        "'system.cpu[0,1,3:8:2].max_insts_all_threads = 42' "
        "sets max_insts_all_threads for cpus 0, 1, 3, 5 and 7 "
        "Direct parameters of the root object are not accessible, "
        "only parameters of its children.",
    )
    parser.add_argument(
        "--bootloader",
        type=str,
        default="",
        help="Bootloader the system uses.",
    )
    parser.add_argument(
        "--dtb",
        type=str,
        default="",
        help="DTB file the system uses.",
    )
    parser.add_argument(
        "--dtbaddr",
        type=int,
        default=0x200000,  # 2MB
        help="DTB load addr.",
    )

    args = parser.parse_args()

    root = Root(full_system=True)
    root.system = create(args)

    root.apply_config(args.param)

    if args.restore is not None:
        m5.instantiate(args.restore)
    else:
        m5.instantiate()

    if args.dtb_gen:
        # No run, autogenerate DTB and exit
        root.system.generateDtb(os.path.join(m5.options.outdir, "system.dtb"))
    else:
        run(args)


if __name__ == "__m5_main__":
    main()
