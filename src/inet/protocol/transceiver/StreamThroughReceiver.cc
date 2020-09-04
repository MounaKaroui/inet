//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#include "inet/protocol/transceiver/StreamThroughReceiver.h"

namespace inet {

Define_Module(StreamThroughReceiver);

void StreamThroughReceiver::initialize(int stage)
{
    StreamingReceiverBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        setTxUpdateSupport(true);
        inputGate->setDeliverImmediately(true);
    }
}

void StreamThroughReceiver::handleMessageWhenUp(cMessage *message)
{
    if (message->getArrivalGate() == inputGate) {
        auto signal = check_and_cast<Signal *>(message);
        if (!signal->isUpdate())
            receivePacketStart(signal, inputGate, datarate);
        else if (signal->getRemainingDuration() == 0)
            receivePacketEnd(signal, inputGate, datarate);
        else {
            auto packet = check_and_cast<Packet *>(signal->getEncapsulatedPacket());
            simtime_t timePosition = signal->getDuration() - signal->getRemainingDuration();
            b position = packet->getTotalLength() * timePosition.dbl() / signal->getDuration().dbl();
            receivePacketProgress(signal, inputGate, datarate, position, timePosition, b(0), 0);
        }
    }
    else
        StreamingReceiverBase::handleMessage(message);
}

void StreamThroughReceiver::handleMessageWhenDown(cMessage *msg)
{
    if (!msg->isSelfMessage()) {
        // received on input gate from another network node
        EV << "Interface is turned off, dropping message (" << msg->getClassName() << ")" << msg->getName() << "\n";
        delete msg;
    }
    else
        OperationalMixin::handleMessageWhenDown(msg);
}

void StreamThroughReceiver::receivePacketStart(cPacket *cpacket, cGate *gate, bps datarate)
{
    ASSERT(rxSignal == nullptr);
    take(cpacket);
    rxSignal = check_and_cast<Signal *>(cpacket);
    emit(receptionStartedSignal, rxSignal);
    auto packet = decodePacket(rxSignal);
    origPacketId = packet->getId();
    pushOrSendPacketStart(packet, outputGate, consumer, datarate);
}

void StreamThroughReceiver::receivePacketProgress(cPacket *cpacket, cGate *gate, bps datarate, b position, simtime_t timePosition, b extraProcessableLength, simtime_t extraProcessableDuration)
{
    take(cpacket);
    if (rxSignal) {
        delete rxSignal;
        rxSignal = check_and_cast<Signal *>(cpacket);
    }
    else {
        EV_WARN << "Dropping signal progress because signal start was not received" << EV_FIELD(signal, *signal) << EV_ENDL;
        delete signal;
    }
}

void StreamThroughReceiver::receivePacketEnd(cPacket *cpacket, cGate *gate, bps datarate)
{
    take(cpacket);
    auto signal = check_and_cast<Signal *>(cpacket);
    emit(receptionEndedSignal, signal);
    if (rxSignal) {
        delete rxSignal;
        rxSignal = nullptr;
        auto packet = decodePacket(signal);
        packet->setOrigPacketId(origPacketId);
        pushOrSendPacketEnd(packet, outputGate, consumer);
        delete signal;
    }
    else {
        EV_WARN << "Dropping signal end because signal start was not received" << EV_FIELD(signal, *signal) << EV_ENDL;
        delete signal;
    }
}

} // namespace inet

