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
	void Peer::OnDelChanProps(TaskResponse response_data, Peer* peer) {
        if (response_data.error_details.response_code != TaskShared::WebErrorCode_Success) {
            ((Peer*)peer)->send_message("PRIVMSG", "Failed to delete chanprops", *server_userSummary, ((Peer*)peer)->m_user_details.nick);
            return;
        }
    }

    void Peer::handle_delchanprops(std::vector<std::string> data_parser) {
        std::string chanmask = data_parser.at(1);

		PeerchatBackendRequest req;

        req.type = EPeerchatRequestType_DeleteChanProps;
		
		req.peer = this;

		ChanpropsRecord chanpropsRecord;
		chanpropsRecord.channel_mask = chanmask;
		req.chanpropsRecord = chanpropsRecord;

		req.peer->IncRef();
		req.callback = OnDelChanProps;
		AddPeerchatTaskRequest(req);

    }
}