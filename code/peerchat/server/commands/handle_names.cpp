#include <OS/OpenSpy.h>

#include <OS/Buffer.h>
#include <OS/KVReader.h>
#include <sstream>
#include <algorithm>

#include <OS/gamespy/gamespy.h>
#include <tasks/tasks.h>


#include <server/Driver.h>
#include <server/Server.h>
#include <server/Peer.h>
namespace Peerchat {
    void Peer::OnNames_FetchChannelInfo(TaskResponse response_data, Peer *peer) {
		std::vector<ChannelUserSummary>::iterator it = response_data.channel_summary.users.begin();

		bool see_invisible = peer->GetOperFlags() & OPERPRIVS_INVISIBLE;		

		if (~peer->GetChannelFlags(response_data.channel_summary.channel_id) & EUserChannelFlag_IsInChannel && !see_invisible) {
			peer->send_numeric(366, "End of /NAMES list.", false, response_data.channel_summary.channel_name);
			return;
		}

		std::ostringstream s;
		std::string target = "= " + response_data.channel_summary.channel_name;
		while (it != response_data.channel_summary.users.end()) {
			ChannelUserSummary user = *(it++);
			if ((user.modeflags & EUserChannelFlag_Invisible) && !see_invisible) {
				continue;
			}
			if(response_data.channel_summary.basic_mode_flags & EChannelMode_Auditorium && user.userSummary.id != peer->GetBackendId()) {
				continue;
			} else if(response_data.channel_summary.basic_mode_flags & EChannelMode_Auditorium_ShowVOP && user.userSummary.id != peer->GetBackendId()) {
				if (!(user.modeflags & (EUserChannelFlag_Owner | EUserChannelFlag_Op | EUserChannelFlag_HalfOp | EUserChannelFlag_Voice))) {
					continue;
				}
			}
			if (user.modeflags & (EUserChannelFlag_Owner | EUserChannelFlag_Op | EUserChannelFlag_HalfOp)) {
				s << "@";
			}
			else if (user.modeflags & EUserChannelFlag_Voice) {
				s << "+";
			}
			s << user.userSummary.nick << " ";
		}
		peer->send_numeric(353, s.str(), false, target);
		peer->send_numeric(366, "End of /NAMES list.", false, response_data.channel_summary.channel_name);
    }
    void Peer::handle_names(std::vector<std::string> data_parser) {
        PeerchatBackendRequest req;
        req.type = EPeerchatRequestType_LookupChannelDetails;
        req.peer = this;
        req.channel_summary.channel_name = data_parser.at(1);
        req.peer->IncRef();
        req.callback = OnNames_FetchChannelInfo;
        AddTaskRequest(req);
    }
}