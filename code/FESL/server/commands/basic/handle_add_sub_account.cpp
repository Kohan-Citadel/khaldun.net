#include <OS/OpenSpy.h>

#include <server/FESLServer.h>
#include <server/FESLDriver.h>
#include <server/FESLPeer.h>


#include <sstream>
namespace FESL {
	bool Peer::m_acct_add_sub_account(OS::KVReader kv_list) {
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

		AddProfileTaskRequest(request);
		
		return true;
	}
	void Peer::m_create_profile_callback(TaskShared::WebErrorDetails error_details, std::vector<OS::Profile> results, std::map<int, OS::User> result_users, void *extra, INetPeer *peer) {
		if (error_details.response_code == TaskShared::WebErrorCode_Success && results.size() > 0) {
			uv_mutex_lock(&((Peer *)peer)->m_mutex);
			((Peer *)peer)->m_profiles.push_back(results.front());
			uv_mutex_unlock(&((Peer *)peer)->m_mutex);
			((Peer *)peer)->SendError(FESL_TYPE_ACCOUNT, FESL_ERROR_NO_ERROR, "AddSubAccount", (int)(ptrdiff_t)extra);
		} else {
			((Peer *)peer)->handle_web_error(error_details, FESL_TYPE_ACCOUNT, "AddSubAccount", (int)(ptrdiff_t)extra);
		}
	}
}