//
// Copyright (C) OpenSim Ltd.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see http://www.gnu.org/licenses/.
//

#include "inet/queueing/sink/ActivePacketSink.h"

namespace inet {
namespace queueing {

Define_Module(ActivePacketSink);

void ActivePacketSink::initialize(int stage)
{
    ActivePacketSinkBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        collectionIntervalParameter = &par("collectionInterval");
        collectionTimer = new cMessage("CollectionTimer");
    }
    else if (stage == INITSTAGE_QUEUEING) {
        if (!collectionTimer->isScheduled() && provider->canPullSomePacket(inputGate->getPathStartGate())) {
            scheduleCollectionTimer();
            collectPacket();
        }
    }
}

void ActivePacketSink::handleMessage(cMessage *message)
{
    if (message == collectionTimer) {
        if (provider->canPullSomePacket(inputGate->getPathStartGate())) {
            scheduleCollectionTimer();
            collectPacket();
        }
    }
    else
        throw cRuntimeError("Unknown message");
}

void ActivePacketSink::scheduleCollectionTimer()
{
    scheduleAfter(collectionIntervalParameter->doubleValue(), collectionTimer);
}

void ActivePacketSink::collectPacket()
{
    auto packet = provider->pullPacket(inputGate->getPathStartGate());
    take(packet);
    EV_INFO << "Collecting packet " << packet->getName() << "." << endl;
    numProcessedPackets++;
    processedTotalLength += packet->getDataLength();
    updateDisplayString();
    dropPacket(packet, OTHER_PACKET_DROP);
}

void ActivePacketSink::handleCanPullPacket(cGate *gate)
{
    Enter_Method("handleCanPullPacket");
    if (!collectionTimer->isScheduled() && provider->canPullSomePacket(inputGate->getPathStartGate())) {
        scheduleCollectionTimer();
        collectPacket();
    }
}

void ActivePacketSink::handlePullPacketProcessed(Packet *packet, cGate *gate, bool successful)
{
    Enter_Method("handlePullPacketProcessed");
}

} // namespace queueing
} // namespace inet

