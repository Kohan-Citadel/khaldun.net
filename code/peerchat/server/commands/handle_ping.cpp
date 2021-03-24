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
    void Peer::handle_ping(std::vector<std::string> data_parser) {
		std::string end = "";
		if (data_parser.size() > 1) {
			end = data_parser.at(1);
			if (end[0] == ':') {
				end = end.substr(1);
			}
		}
        send_message("PONG", end, "", ((Peerchat::Server*)GetDriver()->getServer())->getServerName());

		TaskScheduler<PeerchatBackendRequest, TaskThreadData>* scheduler = ((Peerchat::Server*)(GetDriver()->getServer()))->GetPeerchatTask();
		PeerchatBackendRequest req;
		req.type = EPeerchatRequestType_KeepaliveUser;
		req.summary = GetUserDetails();
		req.peer = this;
		req.peer->IncRef();
		req.callback = NULL;
		scheduler->AddRequest(req.type, req);
    }
}