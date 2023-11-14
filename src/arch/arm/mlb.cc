#include "arch/arm/mlb.hh"
#include "arch/arm/insts/static_inst.hh"
#include "arch/arm/insts/metal.hh"

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

const MRLBEntry MRLB::NullEntry(0, 0, false);

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
    METAL_DBGPRINT(MRLB, ADD, "*added* MRLB entry [mroutine = %d, addr = 0x%lx, valid = %d].\n", ent.getIdx(), ent.getAddr(), ent.isValid());
}

const MRLBEntry & 
MRLB::get(unsigned int idx) const
{
    if (idx > this->size) {
        return NullEntry;
    }

    const MRLBEntry & ent = entries.at(idx);

    if (!ent.isLoaded()) {
        METAL_DBGPRINT(MRLB, GET, "*miss* for mroutine %d.\n", idx);
        return NullEntry;
    }
    
    assert(ent.getIdx() == idx);

    METAL_DBGPRINT(MRLB, GET, "*hit* for mroutine %d, addr = 0x%lx, valid = %d.\n", ent.getIdx(), ent.getAddr(), ent.isValid());
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
    METAL_DBGPRINT(MRLB, FLUSH, "flushing...\n");
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


// IILB
IILBEntry::IILBEntry(const StaticInstPtr _inst, MachInst _opMask, bool _post, unsigned int _mroutine, MachInst _mask0, MachInst _mask1, MachInst _mask2) :
    inst(_inst), armInst(reinterpret_cast<const ArmStaticInst *>(_inst.get())), opMask(_opMask),
    post(_post), mroutine(_mroutine), mask0(_mask0), mask1(_mask1), mask2(_mask2) 
{ 
}

IILBEntry::IILBEntry(const StaticInstPtr _inst, bool _post) : IILBEntry(_inst, 0, _post, 0, 0, 0, 0)
{
}

bool 
IILBEntry::isPost(void) const 
{
    return post;
}

unsigned int 
IILBEntry::getMroutine(void) const 
{
    return mroutine;
}

MachInst 
IILBEntry::getOpMask(void) const 
{
    return opMask;
}

MachInst 
IILBEntry::getMask0(void) const 
{
    return mask0;
}

MachInst 
IILBEntry::getMask1(void) const 
{
    return mask1;
}

MachInst 
IILBEntry::getMask2(void) const 
{
    return mask2;
}

StaticInstPtr IILBEntry::getInst(void) const 
{
    return inst;
}

const ArmStaticInst * 
IILBEntry::getArmStaticInst(void) const 
{
    return this->armInst;
}

bool IILBEntry::isSameInstClass(MachInst s, MachInst d, MachInst opMask)
{
    return (s & ~opMask) == (d & ~opMask);
}

bool IILBEntry::operator==(const IILBEntry &other) const
{
    return this->opMask == other.opMask && this->match(other);
}

bool IILBEntry::match(const IILBEntry &other) const
{
    return isSameInstClass(this->armInst->encoding(), other.armInst->encoding(), this->opMask) && this->post == other.post;
}

const IILBEntry IILB::NullEntry(nullStaticInstPtr, false);

void IILB::flush()
{
    map.clear();
}

IILB::~IILB()
{
    flush();
}

void IILB::add(const IILBEntry & _ent)
{
    auto ent = std::make_unique<IILBEntry>(_ent);

    std::vector<std::unique_ptr<IILBEntry>> * vec;

    auto it = map.find(ent->getInst()->getName());
    if (it == map.end()) {
        auto ptr = std::make_unique<std::vector<std::unique_ptr<IILBEntry>>>();
        vec = ptr.get();
        map.insert({ent->getInst()->getName(), std::move(ptr)});
    } else {
        vec = it->second.get();
    }
    
    // check for duplicates
    auto vit = vec->begin();
    while (vit != vec->end()) {
        auto other = (*vit).get();
        if (*other == *ent) {
            vit = vec->erase(vit);
            break;
        } else {
            ++vit;
        }
    }
    
    METAL_DBGPRINT(IILB, ADD, "*added* IILB entry [inst = 0x%x, mnemonic = \"%s\", opMask = 0x%x, post = %d, mroutine = %u, mask0 = 0x%x, mask1 = 0x%x, mask2 = 0x%x].\n", 
                                                                                    ent->getArmStaticInst()->encoding(),
                                                                                    ent->getInst()->getName(),
                                                                                    ent->getOpMask(), ent->isPost(), 
                                                                                    ent->getMroutine(), ent->getMask0(), 
                                                                                    ent->getMask1(), ent->getMask2());
    vec->push_back(std::move(ent));
}

const IILBEntry & IILB::get(const IILBEntry & ent) const
{
    auto it = map.find(ent.getInst()->getName());
    if (it == map.end()) {
        return NullEntry;
    }

    auto vec = it->second.get();
    auto vit = vec->begin();
    while (vit != vec->end()) {
        auto each = (*vit).get();
        if (each->match(ent)) {
            METAL_DBGPRINT(IILB, GET, "*matched* inst = 0x%x, mnemonic = \"%s\", post = %d -> IILB entry [inst = 0x%x, mnemonic = \"%s\", opMask = 0x%x, post = %d, mroutine = %u, mask0 = 0x%x, mask1 = 0x%x, mask2 = 0x%x].\n",
                                                                                    ent.getArmStaticInst()->encoding(),
                                                                                    ent.getInst()->getName().c_str(),
                                                                                    ent.isPost(), 
                                                                                    each->getArmStaticInst()->encoding(),
                                                                                    each->getInst()->getName().c_str(), 
                                                                                    each->getOpMask(), each->isPost(), 
                                                                                    each->getMroutine(), each->getMask0(), 
                                                                                    each->getMask1(), each->getMask2());
            return *each;
        }
        ++vit;
    }
    return NullEntry;
}
}

} // namespace gem5
