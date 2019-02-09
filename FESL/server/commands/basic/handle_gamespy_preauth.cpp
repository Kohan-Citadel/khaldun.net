#include <OS/OpenSpy.h>

#include <OS/SharedTasks/tasks.h>
#include <server/FESLServer.h>
#include <server/FESLDriver.h>
#include <server/FESLPeer.h>


#include <sstream>
namespace FESL {
	void Peer::m_create_auth_ticket(bool success, OS::User user, OS::Profile profile, TaskShared::AuthData auth_data, void *extra, INetPeer *peer) {
		std::ostringstream s;
		if (success) {
			s << "TXN=GameSpyPreAuth\n";
			if(auth_data.response_proof.length())
				s << "challenge=" << auth_data.response_proof << "\n";
			s << "ticket=" << auth_data.session_key << "\n";
			((Peer *)peer)->SendPacket(FESL_TYPE_ACCOUNT, s.str());
		}
		else {
			((Peer *)peer)->SendError(FESL_TYPE_ACCOUNT, FESL_ERROR_AUTH_FAILURE, "GameSpyPreAuth");
		}
	}
	bool Peer::m_acct_gamespy_preauth(OS::KVReader kv_list) {
		TaskShared::AuthRequest request;
		request.type = TaskShared::EAuthType_MakeAuthTicket;
		request.callback = m_create_auth_ticket;
		request.peer = this;
		IncRef();
		request.profile = m_profile;

		TaskScheduler<TaskShared::AuthRequest, TaskThreadData> *scheduler = ((FESL::Server *)(GetDriver()->getServer()))->GetAuthTask();
		scheduler->AddRequest(request.type, request);
		return true;
	}
}