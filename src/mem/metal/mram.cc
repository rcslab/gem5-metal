/*
 * Copyright (c) 2010-2013, 2015 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2001-2005 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "mem/metal/mram.hh"
#include "base/cprintf.hh"
#include "base/types.hh"
#include"params/MRAM.hh"
#include "base/trace.hh"
#include "debug/Drain.hh"
#include "debug/MRAM.hh"

namespace gem5
{

namespace memory
{

namespace metal
{

MRAM::MRAM(const MRAMParams &p) :
    memory::AbstractMemory(p),
    latency(p.latency),
    retryResp(false),
    dequeueEvent([this]{ dequeue(); }, name())
{
    for (int i = 0; i < p.port_port_connection_count; i++) {
        ports.emplace_back(name() + csprintf(".port[%d]", i), *this);
    }
}

void
MRAM::init()
{
    AbstractMemory::init();

    // allow unconnected memories as this is used in several ruby
    // systems at the moment
    for (const auto & port : ports) {
        if (port.isConnected()) {
            port.sendRangeChange();
        }
    }
}

Tick
MRAM::recvAtomic(PacketPtr pkt)
{
    panic_if(pkt->cacheResponding(), "Should not see packets where cache "
             "is responding");

    access(pkt);
    return getLatency();
}

Tick
MRAM::recvAtomicBackdoor(PacketPtr pkt, MemBackdoorPtr &_backdoor)
{
    Tick latency = recvAtomic(pkt);
    getBackdoor(_backdoor);
    return latency;
}

void
MRAM::recvFunctional(PacketPtr pkt)
{
    pkt->pushLabel(name());

    functionalAccess(pkt);

    bool done = false;
    auto p = packetQueue.begin();
    // potentially update the packets in our packet queue as well
    while (!done && p != packetQueue.end()) {
        done = pkt->trySatisfyFunctional(p->pkt);
        ++p;
    }

    pkt->popLabel();
}

void
MRAM::recvMemBackdoorReq(const MemBackdoorReq &req,
        MemBackdoorPtr &_backdoor)
{
    getBackdoor(_backdoor);
}

bool
MRAM::recvTimingReq(PacketPtr pkt, ResponsePort& port)
{
    panic_if(pkt->cacheResponding(), "Should not see packets where cache "
             "is responding");

    panic_if(!(pkt->isRead() || pkt->isWrite()),
             "Should only see read and writes at memory controller, "
             "saw %s to %#llx\n", pkt->cmdString(), pkt->getAddr());

    // go ahead and deal with the packet and put the response in the
    // queue if there is one
    bool needsResponse = pkt->needsResponse();
    recvAtomic(pkt);
    // turn packet around to go back to requestor if response expected
    if (needsResponse) {
        // recvAtomic() should already have turned packet into
        // atomic response
        assert(pkt->isResponse());

        Tick when_to_send = getLatency();

        // typically this should be added at the end, so start the
        // insertion sort with the last element, also make sure not to
        // re-order in front of some existing packet with the same
        // address, the latter is important as this memory effectively
        // hands out exclusive copies (shared is not asserted)
        auto i = packetQueue.end();
        --i;
        while (i != packetQueue.begin() && when_to_send < i->tick &&
               !i->pkt->matchAddr(pkt))
            --i;

        // emplace inserts the element before the position pointed to by
        // the iterator, so advance it one step
        packetQueue.emplace(++i, pkt, when_to_send, port);

        if (!retryResp && !dequeueEvent.scheduled()) {
            schedule(dequeueEvent, packetQueue.front().tick);
            DPRINTF(MRAM, "scheduled dequeue event at tick %llu for %s to %s.\n", 
                packetQueue.front().tick, 
                pkt->cmdString(),
                pkt->getAddrRange().to_string());
        }
    } else {
        pendingDelete.reset(pkt);
    }

    return true;
}

void
MRAM::dequeue()
{
    assert(!packetQueue.empty());
    DeferredPacket deferred_pkt = packetQueue.front();

    DPRINTF(MRAM, "trying to send response for %s to %s.\n", 
                deferred_pkt.pkt->cmdString(),
                deferred_pkt.pkt->getAddrRange().to_string());
    retryResp = !deferred_pkt.port.sendTimingResp(deferred_pkt.pkt);

    if (!retryResp) {
        DPRINTF(MRAM, "response sent!\n");
        packetQueue.pop_front();
        // if the queue is not empty, schedule the next dequeue event,
        // otherwise signal that we are drained if we were asked to do so
        if (!packetQueue.empty()) {
            // if there were packets that got in-between then we
            // already have an event scheduled, so use re-schedule
            reschedule(dequeueEvent,
                       std::max(packetQueue.front().tick, curTick()), true);
        } else if (drainState() == DrainState::Draining) {
            DPRINTF(Drain, "Draining of MRAM complete\n");
            signalDrainDone();
        }
    }
}

Tick
MRAM::getLatency() const
{
    return clockEdge(latency);
}

void
MRAM::recvRespRetry()
{
    assert(retryResp);

    dequeue();
}

Port &
MRAM::getPort(const std::string &if_name, PortID idx)
{
    if (if_name == "port" && idx >= 0 && idx < ports.size()) {
        return ports.at(idx);
    } else {
        return AbstractMemory::getPort(if_name, idx);
    }
}

DrainState
MRAM::drain()
{
    if (!packetQueue.empty()) {
        DPRINTF(Drain, "MRAM Queue has requests, waiting to drain\n");
        return DrainState::Draining;
    } else {
        return DrainState::Drained;
    }
}

MRAM::CPUPort::CPUPort(const std::string& _name,
                                     MRAM& _memory)
    : ResponsePort(_name), mem(_memory)
{ }

AddrRangeList
MRAM::CPUPort::getAddrRanges() const
{
    AddrRangeList ranges;
    ranges.push_back(mem.getAddrRange());
    return ranges;
}

Tick
MRAM::CPUPort::recvAtomic(PacketPtr pkt)
{
    return mem.recvAtomic(pkt);
}

Tick
MRAM::CPUPort::recvAtomicBackdoor(
        PacketPtr pkt, MemBackdoorPtr &_backdoor)
{
    return mem.recvAtomicBackdoor(pkt, _backdoor);
}

void
MRAM::CPUPort::recvFunctional(PacketPtr pkt)
{
    mem.recvFunctional(pkt);
}

void
MRAM::CPUPort::recvMemBackdoorReq(const MemBackdoorReq &req,
        MemBackdoorPtr &backdoor)
{
    mem.recvMemBackdoorReq(req, backdoor);
}

bool
MRAM::CPUPort::recvTimingReq(PacketPtr pkt)
{
    return mem.recvTimingReq(pkt, *this);
}

void
MRAM::CPUPort::recvRespRetry()
{
    mem.recvRespRetry();
}

}
} // namespace memory
} // namespace gem5
