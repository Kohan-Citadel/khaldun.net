#include <OS/OpenSpy.h>

#include <OS/SharedTasks/tasks.h>
#include <server/FESLServer.h>
#include <server/FESLDriver.h>
#include <server/FESLPeer.h>


#include <sstream>
namespace FESL {
	bool Peer::m_acct_add_persona(OS::KVReader kv_list) {
		TaskShared::ProfileRequest request;
		std::string nick, oldnick;
		nick = kv_list.GetValue("name");

		request.user_search_details.id = m_user.id;
		//request.profile_search_details.id = m_profile.id;
		request.profile_search_details.namespaceid = FESL_PROFILE_NAMESPACEID;
		request.profile_search_details.nick = nick;
		request.profile_search_details.uniquenick = nick;
		int tid = -1;
		if(kv_list.HasKey("TID")) {
			tid = kv_list.GetValueInt("TID");
		}
		request.extra = (void *)(ptrdiff_t)tid;
		request.peer = this;
		request.peer->IncRef();
		request.type = TaskShared::EProfileSearch_CreateProfile;
		request.callback = Peer::m_create_profile_callback;

		TaskScheduler<TaskShared::ProfileRequest, TaskThreadData> *scheduler = ((FESL::Server *)(GetDriver()->getServer()))->GetProfileTask();
		scheduler->AddRequest(request.type, request);
		
		return true;
	}
	void Peer::m_create_nu_profile_callback(TaskShared::WebErrorDetails error_details, std::vector<OS::Profile> results, std::map<int, OS::User> result_users, void *extra, INetPeer *peer) {
		if (error_details.response_code == TaskShared::WebErrorCode_Success && results.size() > 0) {
			((Peer *)peer)->mp_mutex->lock();
			((Peer *)peer)->m_profiles.push_back(results.front());
			((Peer *)peer)->mp_mutex->unlock();
			((Peer *)peer)->SendError(FESL_TYPE_ACCOUNT, FESL_ERROR_NO_ERROR, "NuAddPersona", (int)(ptrdiff_t)extra);
		} else {
			((Peer *)peer)->handle_web_error(error_details, FESL_TYPE_ACCOUNT, "NuAddPersona", (int)(ptrdiff_t)extra);
		}
	}
}