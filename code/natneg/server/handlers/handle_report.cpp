#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include "../NNServer.h"
#include "../../tasks/tasks.h"

#include "../NNDriver.h"

#include <jansson.h>

namespace NN {
	void Driver::handle_report_packet(OS::Address from, NatNegPacket *packet) {
		OS::LogText(OS::ELogLevel_Info, "[%s] Got report- client idx: %d, cookie: %d, game: %s, port type: %d, neg result: %d, neg type: %d, nat mapping scheme: %d", from.ToString().c_str(), packet->Packet.Report.clientindex, packet->cookie, packet->Packet.Report.gamename, packet->Packet.Report.porttype, packet->Packet.Report.negResult, packet->Packet.Report.natType, packet->Packet.Report.natMappingScheme);
		packet->packettype = NN_REPORT_ACK;
		SendPacket(from, packet);
	}
    void Driver::send_report_ack(OS::Address to_address, NatNegPacket *packet) {
        OS::LogText(OS::ELogLevel_Info, "[%s] Send Report ack", to_address.ToString().c_str());
		SendPacket(to_address, packet);
	}
}