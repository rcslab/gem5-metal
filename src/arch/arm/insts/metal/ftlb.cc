#include "arch/arm/insts/metal/ftlb.hh"
#include "arch/arm/insts/metal/common.hh"
#include "arch/arm/tlbi_op.hh"
#include "arch/arm/types.hh"
#include "base/types.hh"
#include "cpu/checker/cpu.hh"
#include "cpu/op_class.hh"

namespace gem5 { namespace ArmISA {
namespace metal { namespace inst {
    FTLBOp::FTLBOp(ExceptionLevel _el, bool _secure,
        unsigned int _vmid,
        bool _match_vmid,
        unsigned int _asid,
        bool _match_asid,
        Addr _va,
        bool _match_va)
    : TLBIOp(_el, _secure),
    vmid(_vmid), match_vmid(_match_vmid),
    asid(_asid), match_asid(_match_asid),
    va(_va), match_va(_match_va)
    {
    }

    void FTLBOp::operator()(ThreadContext* tc)
    {
        getMMUPtr(tc)->flush(*this);
        // If CheckerCPU is connected, need to notify it of a flush
        CheckerCPU *checker = tc->getCheckerCpuPtr();
        if (checker) {
            getMMUPtr(checker)->flush(*this);
        }
    }

    std::string FTLBOp::print() const
    {
        std::stringstream ss;
        ss << "EL: " << targetEL <<
            ", secure: " << secureLookup <<
            ", VA: " << va <<
            ", match va: " << match_va <<
            ", asid: " << asid <<
            ", match asid: " << match_asid <<
            ", vmid: " << vmid <<
            ", match vmid: " << match_vmid;
        return ss.str();
    }

    bool FTLBOp::match(TlbEntry *entry, [[maybe_unused]] vmid_t curr_vmid) const
    {
        bool match = (entry->el == this->targetEL) && (entry->nstid == !this->secureLookup);

        if (match && this->match_va) {
            TlbEntry::Lookup lookup;
            lookup.va = va;
            lookup.size = false;
            match &= entry->matchAddress(lookup);
        }

        if (match && this->match_asid) {
            match &= (entry->asid == this->asid);
        }

        if (match && this->match_vmid) {
            match &= (entry->vmid == this->vmid);
        }

        return match;
    };

    bool FTLBOp::stage1Flush() const
    {
        return true;
    }

    bool FTLBOp::stage2Flush() const
    {
        return false;
    }

    FTLBOp FTLBOp::makeStage2() const
    {
        panic("unimplemented!");
    }

    Ftlb64::Ftlb64(ExtMachInst _machInst, RegIndex _r1, RegIndex _r2) :
        MetalReg2Op("ftlb", _machInst, IntAluOp, _r1, _r2)
    {
        setSrcRegIdx(_numSrcRegs++, intRegClass[r1]);
        setSrcRegIdx(_numSrcRegs++, intRegClass[r2]);

        this->flags[IsInteger] = true;
    }

    Fault Ftlb64::execute(ExecContext *xc, trace::InstRecord *traceData) const
    {
        const Addr vaddr = xc->getRegOperand(this, 0);
        FTLBAttr attrs = static_cast<FTLBAttr>(xc->getRegOperand(this, 1));

        if (attrs.el > ExceptionLevel::EL3) {
            METAL_DBGPRINT(INSTS, FTLB, "Invalid Attributes: #lx.\n", attrs);
            return std::make_shared<SupervisorTrap>(machInst, 0, ExceptionClass::TRAPPED_METAL_ACCESS);
        }

        FTLBOp ftlb_op(static_cast<ExceptionLevel>(static_cast<int>(attrs.el)), !attrs.nstid,
                      attrs.vmid, attrs.match_vmid,
                      attrs.asid, attrs.match_asid,
                      vaddr, attrs.match_va);

        if (attrs.broadcast) {
            ftlb_op.broadcast(xc->tcBase());
        } else {
            ftlb_op(xc->tcBase());
        }

        return NoFault;
    }
}}
}}
