#include <OS/OpenSpy.h>

#include <OS/Buffer.h>
#include <OS/KVReader.h>
#include <sstream>
#include <algorithm>

#include <OS/gamespy/gamespy.h>
#include <OS/SharedTasks/tasks.h>
#include <tasks/tasks.h>


#include <server/Driver.h>
#include <server/Server.h>
#include <server/Peer.h>

namespace Peerchat {
	void Peer::OnJoinChannel(TaskResponse response_data, Peer* peer) {
		if (response_data.error_details.response_code == TaskShared::WebErrorCode_Success) {
			peer->mp_mutex->lock();
			peer->m_channel_flags[response_data.channel_summary.channel_id] |= EUserChannelFlag_IsInChannel;
			peer->mp_mutex->unlock();
		}
	}
    void Peer::handle_join(std::vector<std::string> data_parser) {
        std::string target = data_parser.at(1);

        TaskScheduler<PeerchatBackendRequest, TaskThreadData> *scheduler = ((Peerchat::Server *)(GetDriver()->getServer()))->GetPeerchatTask();
        PeerchatBackendRequest req;
        req.type = EPeerchatRequestType_UserJoinChannel;
        req.peer = this;
        req.channel_summary.channel_name = target;
        
        req.peer->IncRef();
        req.callback = OnJoinChannel;
        scheduler->AddRequest(req.type, req);
        
    }
}