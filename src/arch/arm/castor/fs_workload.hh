#ifndef __ARCH_ARM_CASTOR_FS_WORKLOAD_HH__
#define __ARCH_ARM_CASTOR_FS_WORKLOAD_HH__

#include <map>

#include "arch/arm/fs_workload.hh"
#include "params/ArmFsCastor.hh"

namespace gem5
{

namespace ArmISA
{

class FsCastor : public ArmISA::FsWorkload
{
  public:
    /** Boilerplate params code */
    PARAMS(ArmFsCastor);

    FsCastor(const Params &p);
    ~FsCastor() = default;

    void initState() override;
};

} // namespace ArmISA
} // namespace gem5

#endif // __ARCH_ARM_FREEBSD_FS_WORKLOAD_HH__
