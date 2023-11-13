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
    MRLBEntry(void);
    MRLBEntry(unsigned int idx, Addr addr, bool valid);
    ~MRLBEntry(void) = default;
    DISALLOW_COPY_AND_ASSIGN(MRLBEntry);

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
    DISALLOW_COPY_AND_ASSIGN(MRLB);
    MRLB() = delete;
    MRLB(unsigned int _size);
    ~MRLB() = default;

    static const MRLBEntry NullEntry;
    const MRLBEntry & get(unsigned int idx) const;
    void add(const MRLBEntry & ent);
    unsigned int getSize(void) const;

    void flushAll(void);
    void flush(unsigned int idx);
};

class ArmStaticInst;

class IILBEntry
{
private:
    const StaticInstPtr inst;
    const ArmStaticInst * armInst;
    const MachInst opMask;
    const bool post;
    const unsigned int mroutine;
    const MachInst mask0;
    const MachInst mask1;
    const MachInst mask2;
public:
    IILBEntry(void) = delete;
    IILBEntry(const StaticInstPtr _inst, MachInst _opMask, bool _post, unsigned int _mroutine, MachInst _mask0, MachInst _mask1, MachInst _mask2);
    IILBEntry(const StaticInstPtr _inst, bool _post);

    bool isPost(void) const;
    unsigned int getMroutine(void) const;
    MachInst getOpMask(void) const;
    MachInst getMask0(void) const;
    MachInst getMask1(void) const;
    MachInst getMask2(void) const;
    StaticInstPtr getInst(void) const;
    const ArmStaticInst * getArmStaticInst(void) const;
  
    static bool isSameInstClass(MachInst s, MachInst d, MachInst opMask);
    bool match(const IILBEntry &other) const;
    bool operator==(const IILBEntry &other) const;
};

class IILB
{
private:
    std::unordered_map<std::string, std::unique_ptr<std::vector<std::unique_ptr<IILBEntry>>> > map;
public:
    DISALLOW_COPY_AND_ASSIGN(IILB);
    ~IILB(void);
    IILB(void) = default;
    static const IILBEntry NullEntry;
    
    void add(const IILBEntry & _ent);
    const IILBEntry & get(const IILBEntry & ent) const;
    void flush(void);
};


} // namespace ArmISA
} // namespace gem5

#endif // __ARCH_ARM_MLB_HH__
