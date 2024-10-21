#include "arch/arm/castor/fs_workload.hh"

#include "arch/arm/utility.hh"
#include "base/loader/dtb_file.hh"
#include "base/loader/object_file.hh"
#include "base/loader/symtab.hh"
#include "cpu/base.hh"
#include "cpu/pc_event.hh"
#include "cpu/thread_context.hh"
#include "debug/Loader.hh"
#include "mem/physical.hh"
#include "sim/stat_control.hh"

namespace gem5
{

namespace ArmISA
{

FsCastor::FsCastor(const Params &p) : ArmISA::FsWorkload(p)
{
}

void
FsCastor::initState()
{
    FsWorkload::initState();

    fatal_if(params().dtb_filename == "", "dtb file is not specified.");
    fatal_if(params().dtb_addr == 0, "dtb load addr is not specified.");

    // Using Device Tree Blob to describe system configuration.
    inform("Loading DTB file: %s at address %#x\n", params().dtb_filename,
        params().dtb_addr);

    auto *dtb_file = new loader::DtbFile(params().dtb_filename);

    dtb_file->buildImage().
        offset(params().dtb_addr).
        write(system->physProxy);

    for (auto *tc: system->threads) {
        tc->setReg(int_reg::R0, (RegVal)0x10CA5201); // magic number
        inform("Setting R0 to magic number 0x10CA5201.\n");
        tc->setReg(int_reg::R1, params().dtb_addr);
        inform("Setting R1 to DTB load address %#x.\n", params().dtb_addr);
        tc->setReg(int_reg::R2, dtb_file->getLength());
        inform("Setting R2 to DTB size %#x.\n", dtb_file->getLength());
    }

    delete dtb_file;
}

} // namespace ArmISA
} // namespace gem5
