#ifndef __ARCH_ARM_MLB_HH__
#define __ARCH_ARM_MLB_HH__

#include <vector>
#include "arch/arm/system.hh"
#include "arch/arm/types.hh"

namespace gem5
{

namespace ArmISA {

class MRLB;

class MRLBEntry {
  private:
    unsigned int idx;
    Addr addr;
    bool valid;
    bool loaded;
  protected:
    // readonly to other classes
    bool isLoaded(void) const;
    void setValid(bool val);
    void setLoaded(bool val);
    void setAddr(Addr addr);
    void setIdx(unsigned int idx);
    void set(const MRLBEntry & other);
  public:
    MRLBEntry();
    MRLBEntry(unsigned int idx, Addr addr, bool valid);
    ~MRLBEntry() = default;
    bool isValid(void) const;
    Addr getAddr(void) const;
    unsigned int getIdx(void) const;

  friend class MRLB;
};

class MRLB {
  private:
    unsigned int size;
    std::vector<MRLBEntry> entries;

  public:
    static const MRLBEntry NullMRLBEntry;

    MRLB() = delete;
    MRLB(unsigned int _size);
    ~MRLB() = default;

    const MRLBEntry & get(unsigned int idx) const;
    void add(const MRLBEntry & ent);
    unsigned int getSize(void) const;

    void flushAll(void);
    void flush(unsigned int idx);
};

} // namespace ArmISA
} // namespace gem5

#endif // __ARCH_ARM_MLB_HH__
