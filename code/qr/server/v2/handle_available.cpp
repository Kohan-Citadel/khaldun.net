#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <server/QRServer.h>
#include <server/QRDriver.h>
#include <qr/tasks/tasks.h>
#include <tasks/tasks.h>
#include <server/v2.h>
namespace QR {
    void Driver::on_got_v2_available_data(MM::MMTaskResponse response) {
		OS::Buffer buffer;

		uv_timespec64_t current_time;
		uv_clock_gettime(UV_CLOCK_MONOTONIC, &current_time);

		buffer.WriteByte(QR_MAGIC_1);
		buffer.WriteByte(QR_MAGIC_2);

		buffer.WriteByte(PACKET_AVAILABLE);
		buffer.WriteInt(htonl(response.game_data.disabled_services));
		response.driver->SendUDPPacket(response.from_address, buffer);
    }

    void Driver::handle_v2_available(OS::Address from_address, uint8_t *instance_key, OS::Buffer &buffer) {
        std::string gamename = buffer.ReadNTS();

		MM::MMPushRequest req;

		req.gamename = gamename;
        req.callback = on_got_v2_available_data;
        req.from_address = from_address;
        req.v2_instance_key = *(uint32_t *)instance_key;
        req.driver = this;
		req.version = 2;

		OS::LogText(OS::ELogLevel_Info, "[%s] Got available request: %s", from_address.ToString().c_str(), req.gamename.c_str());
		req.type = MM::EMMPushRequestType_GetGameInfoByGameName;
		AddRequest(req);
    }
}