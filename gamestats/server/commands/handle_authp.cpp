#include <server/GSServer.h>
#include <server/GSPeer.h>
#include <server/GSDriver.h>
#include <OS/OpenSpy.h>
#include <OS/Buffer.h>
#include <OS/Search/Profile.h>

#include <OS/Search/User.h>

#include <stddef.h>
#include <sstream>
#include <algorithm>

#include <server/tasks/tasks.h>

#include <OS/GPShared.h>

#include <map>
#include <utility>

using namespace GPShared;

namespace GS {
	void Peer::perform_pid_auth(int profileid, const char *response, int operation_id) {
		TaskScheduler<PersistBackendRequest, TaskThreadData> *scheduler = ((GS::Server *)(GetDriver()->getServer()))->GetGamestatsTask();
		PersistBackendRequest req;
		req.mp_peer = this;
		req.mp_extra = (void *)operation_id;
		req.type = EPersistRequestType_Auth_ProfileID;
		req.callback = getPersistDataCallback;
		req.profileid = profileid;
		req.game_instance_identifier = response;
		req.modified_since = m_session_key;
		req.authCallback = m_nick_email_auth_cb;
		IncRef();
		scheduler->AddRequest(req.type, req);
	}
	void Peer::perform_preauth_auth(std::string auth_token, const char *response, int operation_id) {
		TaskScheduler<PersistBackendRequest, TaskThreadData> *scheduler = ((GS::Server *)(GetDriver()->getServer()))->GetGamestatsTask();
		PersistBackendRequest req;
		req.mp_peer = this;
		req.mp_extra = (void *)operation_id;
		req.type = EPersistRequestType_Auth_AuthTicket;
		req.callback = getPersistDataCallback;
		req.auth_token = auth_token;
		req.game_instance_identifier = response;
		req.modified_since = m_session_key;
		req.data_index = operation_id;
		req.authCallback = m_nick_email_auth_cb;
		IncRef();
		scheduler->AddRequest(req.type, req);
	}
	void Peer::perform_cdkey_auth(std::string cdkey, std::string response, int operation_id) {
		TaskScheduler<PersistBackendRequest, TaskThreadData> *scheduler = ((GS::Server *)(GetDriver()->getServer()))->GetGamestatsTask();
		PersistBackendRequest req;
		req.mp_peer = this;
		req.mp_extra = (void *)operation_id;
		req.type = EPersistRequestType_Auth_CDKey;
		req.callback = getPersistDataCallback;
		req.auth_token = cdkey;
		req.game_instance_identifier = response;
		req.modified_since = m_session_key;
		req.data_index = operation_id;
		req.authCallback = m_nick_email_auth_cb;
		IncRef();
		scheduler->AddRequest(req.type, req);
	}
	void Peer::handle_authp(OS::KVReader data_parser) {
		// TODO: CD KEY AUTH
		int pid = data_parser.GetValueInt("pid");

		int operation_id = data_parser.GetValueInt("lid");

		std::string resp;
		if (!data_parser.HasKey("resp")) {
			send_error(GPShared::GP_PARSE);
			return;
		}

		resp = data_parser.GetValue("resp");

		if (operation_id > m_last_authp_operation_id) {
			m_last_authp_operation_id = operation_id;
		}

		if(pid != 0) {
			perform_pid_auth(pid, resp.c_str(), operation_id);
		} else if(data_parser.HasKey("keyhash")) {
			perform_cdkey_auth(data_parser.GetValue("keyhash"), resp, operation_id);
		} else {
			if (data_parser.HasKey("authtoken")) {
				std::string auth_token = data_parser.GetValue("authtoken");
				perform_preauth_auth(auth_token, resp.c_str(), operation_id);
				return;
			}
			send_error(GPShared::GP_PARSE);
		}

	}
	void Peer::m_nick_email_auth_cb(bool success, OS::User user, OS::Profile profile, TaskShared::AuthData auth_data, void *extra, INetPeer *peer) {

		((Peer *)peer)->mp_mutex->lock();
		if(!((Peer *)peer)->m_backend_session_key.length() && auth_data.session_key.length())
			((Peer *)peer)->m_backend_session_key = auth_data.session_key;


		std::ostringstream ss;
		int operation_id = (int)extra;

		if (operation_id >= ((Peer *)peer)->m_last_authp_operation_id) {
			((Peer *)peer)->m_user = user;
			((Peer *)peer)->m_profile = profile;
		}

		((Peer *)peer)->m_authenticated_profileids.push_back(profile.id);


		if(success) {
			ss << "\\pauthr\\" << profile.id;

			if(auth_data.session_key.length()) {
				ss << "\\lt\\" << auth_data.session_key;
			}
			ss << "\\lid\\" << operation_id;

			((Peer *)peer)->m_profile = profile;
			((Peer *)peer)->m_user = user;

			((Peer *)peer)->SendPacket(ss.str());

		} else {
			ss << "\\pauthr\\-1\\errmsg\\";
			GPShared::GPErrorData error_data;
			GPShared::GPErrorCode code;

			switch(auth_data.response_code) {
				case OS::LOGIN_RESPONSE_USER_NOT_FOUND:
					//code = GP_LOGIN_BAD_EMAIL;
					code = GP_LOGIN_BAD_PROFILE;
					break;
				case OS::LOGIN_RESPONSE_INVALID_PASSWORD:
					code = GP_LOGIN_BAD_PASSWORD;
				break;
				case OS::LOGIN_RESPONSE_INVALID_PROFILE:
					code = GP_LOGIN_BAD_PROFILE;
				break;
				case OS::LOGIN_RESPONSE_UNIQUE_NICK_EXPIRED:
					code = GP_LOGIN_BAD_UNIQUENICK;
				break;
				default:
				case OS::LOGIN_RESPONSE_DB_ERROR:
					code = GP_DATABASE;
				break;
				case OS::LOGIN_RESPONSE_SERVERINITFAILED:
				case OS::LOGIN_RESPONSE_SERVER_ERROR:
					code = GP_NETWORK;
			}

			error_data = GPShared::getErrorDataByCode(code);

			ss << error_data.msg;

			ss << "\\lid\\" << operation_id;
			((Peer *)peer)->SendPacket(ss.str());
		}
		((Peer *)peer)->mp_mutex->unlock();
	}
}