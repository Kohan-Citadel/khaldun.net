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
	void Peer::OnDeleteUserMode(TaskResponse response_data, Peer* peer) {
		printf("deleteusermode\n");
	}

    void Peer::handle_delusermode(std::vector<std::string> data_parser) {
        std::string usermodeid_string = data_parser.at(1);

		TaskScheduler<PeerchatBackendRequest, TaskThreadData>* scheduler = ((Peerchat::Server*)(GetDriver()->getServer()))->GetPeerchatTask();
		PeerchatBackendRequest req;

		req.type = EPeerchatRequestType_DeleteUserMode;
		req.peer = this;

		UsermodeRecord usermodeRecord;
		usermodeRecord.usermodeid = atoi(usermodeid_string.c_str());
		usermodeRecord.setByUserSummary = GetUserDetails();
		req.usermodeRecord = usermodeRecord;

		req.peer->IncRef();
		req.callback = OnDeleteUserMode;
		scheduler->AddRequest(req.type, req);

    }
}