#include "tasks.h"
#include <sstream>
#include <server/Server.h>

namespace Peerchat {
    bool Perform_UserPartChannel(PeerchatBackendRequest request, TaskThreadData *thread_data) {
        ChannelSummary channel = GetChannelSummaryByName(thread_data, request.channel_summary.channel_name, false);
        RemoveUserFromChannel(thread_data, request.summary, channel, "PART", request.message);

        int from_mode_flags = LookupUserChannelModeFlags(thread_data, channel.channel_id, request.peer->GetBackendId());

		if(from_mode_flags & EUserChannelFlag_Invisible) {
			std::ostringstream mq_message;
			std::string message = "INVISIBLE USER " + request.summary.nick + " PARTED CHANNEL";
			const char* base64 = OS::BinToBase64Str((uint8_t*)message.c_str(), message.length());
			std::string b64_string = base64;
			free((void*)base64);

			mq_message << "\\type\\NOTICE\\toChannelId\\" << channel.channel_id << "\\message\\" << b64_string << "\\fromUserSummary\\" << request.summary.ToString(true) << "\\requiredChanUserModes\\" << EUserChannelFlag_Invisible << "\\includeSelf\\1";
			thread_data->mp_mqconnection->sendMessage(peerchat_channel_exchange, peerchat_client_message_routingkey, mq_message.str().c_str());
		}

		TaskResponse response;
        if(request.callback)
            request.callback(response, request.peer);
		if (request.peer)
			request.peer->DecRef();
        return true;
    }
}