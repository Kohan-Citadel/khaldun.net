#include <OS/OpenSpy.h>

#include <OS/SharedTasks/tasks.h>
#include <server/FESLServer.h>
#include <server/FESLDriver.h>
#include <server/FESLPeer.h>

#include <server/tasks/tasks.h>


#include <sstream>
namespace FESL {
	void Peer::m_login_fetched_game_entitlements_auth_cb(TaskShared::WebErrorDetails error_details, std::vector<EntitledGameFeature> results, INetPeer *peer, void *extra) {
			if(error_details.response_code != TaskShared::WebErrorCode_Success) {
				((Peer *)peer)->handle_web_error(error_details, FESL_TYPE_ACCOUNT, "Login", (int)extra);
				return;
			}
			std::ostringstream s;

			s << "TXN=Login\n";
			if((int)extra != -1) {
				s << "TID=" << (int)extra << "\n";
			}
			s << "lkey=" << ((Peer *)peer)->m_session_key << "\n";
			
			s << "displayName=" << ((Peer *)peer)->m_account_profile.uniquenick << "\n";
			s << "userId=" << ((Peer *)peer)->m_user.id << "\n";
			s << "profileId=" << ((Peer *)peer)->m_account_profile.id << "\n";
			if (((Peer *)peer)->m_encrypted_login_info.length()) {
				s << "encryptedLoginInfo=" << OS::url_encode(((Peer *)peer)->m_encrypted_login_info) << "\n";
			}

			int idx = 0;
			if(results.size() > 0) {
				s << "entitledGameFeatureWrappers.[]=" << results.size() << "\n";
				std::vector<EntitledGameFeature>::iterator it = results.begin();
				while(it != results.end()) {
					EntitledGameFeature feature = *it;


					char timeBuff[128];
					struct tm *newtime;
					newtime = gmtime((time_t *)&feature.EntitlementExpirationDate);

					strftime(timeBuff, sizeof(timeBuff), FESL_DATE_FORMAT, newtime);

					 s << "entitledGameFeatureWrappers." << idx << ".gameFeatureId=" << feature.GameFeatureId << "\n";
					 s << "entitledGameFeatureWrappers." << idx << ".status=" << feature.Status << "\n";
					 s << "entitledGameFeatureWrappers." << idx << ".entitlementExpirationDays=" << feature.EntitlementExpirationDays << "\n";


					 s << "entitledGameFeatureWrappers." << idx << ".entitlementExpirationDate=";// <<  << "\n"; //XXX: convert to encoded string
					 if(feature.EntitlementExpirationDate != 0) {
						s << "\""  << OS::url_encode(timeBuff) << "\"";
					 }
					 s << "\n";

					 s << "entitledGameFeatureWrappers." << idx << ".message=";
					 if(feature.Message.length() > 0) {
							s << "\""  << OS::url_encode(feature.Message) << "\"";
					 }
					 s << "\n";
					idx++;
					it++;
				}
			}


			((Peer *)peer)->SendPacket(FESL_TYPE_ACCOUNT, s.str());


			TaskShared::ProfileRequest request;
			request.type = TaskShared::EProfileSearch_Profiles;
			request.user_search_details.id = ((Peer *)peer)->m_user.id;
			request.user_search_details.partnercode = OS_EA_PARTNER_CODE;
			request.profile_search_details.namespaceid = FESL_PROFILE_NAMESPACEID;
			request.extra = extra;
			request.peer = peer;
			peer->IncRef();
			request.callback = Peer::m_search_callback;
			TaskScheduler<TaskShared::ProfileRequest, TaskThreadData> *scheduler = ((FESL::Server *)(peer->GetDriver()->getServer()))->GetProfileTask();
			scheduler->AddRequest(request.type, request);

	}
	void Peer::m_login_auth_cb(bool success, OS::User user, OS::Profile profile, TaskShared::AuthData auth_data, void *extra, INetPeer *peer) {
		std::ostringstream s;
		if (success) {
			((Peer *)peer)->m_logged_in = true;
			((Peer *)peer)->m_session_key = auth_data.session_key;
			((Peer *)peer)->m_user = user;
			((Peer *)peer)->m_account_profile = profile;
			
			
			PublicInfo server_info = ((FESL::Driver *)peer->GetDriver())->GetServerInfo();
			
			FESLRequest request;
			request.type = EFESLRequestType_GetEntitledGameFeatures;
			request.peer = peer;
			request.extra = extra;
			request.profileid = profile.id;
			request.driverInfo = server_info;
			peer->IncRef();

			request.gameFeaturesCallback = m_login_fetched_game_entitlements_auth_cb;

			TaskScheduler<FESLRequest, TaskThreadData> *scheduler = ((FESL::Server *)(peer->GetDriver()->getServer()))->GetFESLTasks();
			scheduler->AddRequest(request.type, request);
			

		}
		else {
			((Peer *)peer)->handle_web_error(auth_data.error_details, FESL_TYPE_ACCOUNT, "Login", (int)extra);
		}
	}
	bool Peer::m_acct_login_handler(OS::KVReader kv_list) {
		std::string nick, password;
		if (kv_list.HasKey("encryptedInfo")) {
			m_encrypted_login_info = OS::url_decode(kv_list.GetValue("encryptedInfo"));
			kv_list = ((FESL::Driver *)this->GetDriver())->getStringCrypter()->decryptString(m_encrypted_login_info);
		}

		nick = kv_list.GetValue("name");
		password = kv_list.GetValue("password");

		if (kv_list.GetValueInt("returnEncryptedInfo")) {
			std::ostringstream s;
			s << "\\name\\" << nick;
			s << "\\password\\" << password;
			m_encrypted_login_info = ((FESL::Driver *)this->GetDriver())->getStringCrypter()->encryptString(s.str());
		}

		TaskShared::AuthRequest request;
		request.type = TaskShared::EAuthType_Uniquenick_Password;
		request.callback = m_login_auth_cb;
		request.peer = this;
		request.extra = (void *)-1;
		if(kv_list.HasKey("TID")) {
			request.extra = (void *)kv_list.GetValueInt("TID");
		}

		IncRef();
		request.profile.uniquenick = nick;
		request.profile.namespaceid = FESL_ACCOUNT_NAMESPACEID;
		request.user.partnercode = OS_EA_PARTNER_CODE;
		request.user.password = password;

		TaskScheduler<TaskShared::AuthRequest, TaskThreadData> *scheduler = ((FESL::Server *)(GetDriver()->getServer()))->GetAuthTask();
		scheduler->AddRequest(request.type, request);
		return true;
	}
}