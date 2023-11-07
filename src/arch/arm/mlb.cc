#include "arch/arm/mlb.hh"
#include "debug/Metal.hh"

namespace gem5
{

namespace ArmISA
{

bool MRLBEntry::isLoaded(void) const 
{
    return this->loaded;
}

void MRLBEntry::setValid(bool val)
{
    this->valid = val;
}

void MRLBEntry::setLoaded(bool val)
{
    this->loaded = val;
}

void MRLBEntry::setAddr(Addr addr)
{
    this->addr = addr;
}

unsigned int MRLBEntry::getIdx(void) const
{
    return this->idx;
}

void MRLBEntry::setIdx(unsigned int idx)
{
    this->idx = idx;
}

void MRLBEntry::set(const MRLBEntry & other)
{
    // copy another entrie's public attributes
    this->addr = other.addr;
    this->valid = other.valid;
    this->idx = other.idx;
}

MRLBEntry::MRLBEntry() :
    MRLBEntry(0, 0, false)
{
}

MRLBEntry::MRLBEntry(unsigned int _idx, Addr _addr, bool _valid) : 
    idx(_idx),
    addr(_addr),
    valid(_valid),
    loaded(false)
{
}

bool MRLBEntry::isValid(void) const
{
    return this->valid;
}

Addr MRLBEntry::getAddr(void) const
{
    return this->addr;
}

const MRLBEntry MRLB::NullMRLBEntry(0, 0, false);

MRLB::MRLB(unsigned int _size) : 
    size(_size), entries(std::vector<MRLBEntry>(_size))
{
}

void MRLB::add(const MRLBEntry & e)
{
    if (e.getIdx() > this->size) {
        return;
    }

    MRLBEntry & ent = entries.at(e.getIdx());

    ent.set(e);
    ent.setLoaded(true);
    DPRINTF(Metal, "MRLB: added mroutine %d, addr = 0x%lx, valid = %d...\n", ent.getIdx(), ent.getAddr(), ent.isValid());
}

const MRLBEntry & 
MRLB::get(unsigned int idx) const
{
    if (idx > this->size) {
        return NullMRLBEntry;
    }

    const MRLBEntry & ent = entries.at(idx);

    assert(ent.getIdx() == idx);

    if (!ent.isLoaded()) {
        DPRINTF(Metal, "MRLB: *miss* for mroutine %d.\n", idx);
        return NullMRLBEntry;
    }

    DPRINTF(Metal, "MRLB: *hit* for mroutine %d. Addr = 0x%lx, valid = %d...\n", ent.getIdx(), ent.getAddr(), ent.isValid());
    return ent;
}

unsigned int 
MRLB::getSize(void) const 
{
    return this->size;
}

void 
MRLB::flushAll(void)
{
    DPRINTF(Metal, "MRLB: flushing...\n");
    for (unsigned int i = 0; i < this->size; i++) {
        this->flush(i);
    }
}

void 
MRLB::flush(unsigned int idx)
{
    MRLBEntry & ent = this->entries.at(idx);
    ent.setAddr(0);
    ent.setValid(false);
    ent.setLoaded(false);
}
}

} // namespace gem5
