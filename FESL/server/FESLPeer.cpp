#include "FESLPeer.h"
#include "FESLDriver.h"
#include <OS/OpenSpy.h>
#include <OS/Search/Profile.h>
#include <OS/Search/User.h>
#include <OS/Search/Profile.h>
#include <OS/Auth.h>

#include <sstream>

namespace FESL {
	CommandHandler Peer::m_commands[] = {
		{ FESL_TYPE_FSYS, "Hello", &Peer::m_fsys_hello_handler },
		{ FESL_TYPE_FSYS, "Ping", &Peer::m_fsys_ping_handler },
		{ FESL_TYPE_FSYS, "MemCheck", &Peer::m_fsys_memcheck_handler },
		{ FESL_TYPE_FSYS, "Goodbye", &Peer::m_fsys_goodbye_handler },
		{ FESL_TYPE_SUBS, "GetEntitlementByBundle", &Peer::m_subs_get_entitlement_by_bundle },
		{ FESL_TYPE_DOBJ, "GetObjectInventory", &Peer::m_dobj_get_object_inventory },
		{ FESL_TYPE_ACCOUNT, "Login", &Peer::m_acct_login_handler },
		{ FESL_TYPE_ACCOUNT, "NuLogin", &Peer::m_acct_nulogin_handler },
		{ FESL_TYPE_ACCOUNT, "NuGetPersonas", &Peer::m_acct_get_personas},
		{ FESL_TYPE_ACCOUNT, "NuLoginPersona",  &Peer::m_acct_login_persona },
		{ FESL_TYPE_ACCOUNT, "GetTelemetryToken",  &Peer::m_acct_get_telemetry_token},
		{ FESL_TYPE_ACCOUNT, "RegisterGame", &Peer::m_acct_register_game_handler },
		{ FESL_TYPE_ACCOUNT, "GetCountryList", &Peer::m_acct_get_country_list },
		{ FESL_TYPE_ACCOUNT, "GetTos", &Peer::m_acct_gettos_handler },
		{ FESL_TYPE_ACCOUNT, "GetSubAccounts", &Peer::m_acct_get_sub_accounts },
		{ FESL_TYPE_ACCOUNT, "LoginSubAccount",  &Peer::m_acct_login_sub_account },
		{ FESL_TYPE_ACCOUNT, "AddSubAccount",  &Peer::m_acct_add_sub_account },
		{ FESL_TYPE_ACCOUNT, "AddAccount",  &Peer::m_acct_add_account },
		{ FESL_TYPE_ACCOUNT, "UpdateAccount",  &Peer::m_acct_update_account },
		{ FESL_TYPE_ACCOUNT, "DisableSubAccount",  &Peer::m_acct_disable_sub_account },
		{ FESL_TYPE_ACCOUNT, "GetAccount", &Peer::m_acct_get_account },
		{ FESL_TYPE_ACCOUNT, "GameSpyPreAuth", &Peer::m_acct_gamespy_preauth },
		{ FESL_TYPE_ACCOUNT, "SendAccountName", &Peer::m_acct_send_account_name},
		{ FESL_TYPE_ACCOUNT, "SendPassword", &Peer::m_acct_send_account_password},
	};
	Peer::Peer(Driver *driver, INetIOSocket *sd) : INetPeer(driver, sd) {
		m_delete_flag = false;
		m_timeout_flag = false;

		gettimeofday(&m_last_ping, NULL);
		gettimeofday(&m_last_recv, NULL);
		
		m_sequence_id = 1;
		m_logged_in = false;
		m_pending_subaccounts = false;
		m_got_profiles = false;
		m_pending_nuget_personas = false;

		mp_mutex = OS::CreateMutex();

		OS::LogText(OS::ELogLevel_Info, "[%s] New connection", m_sd->address.ToString().c_str());
	}
	Peer::~Peer() {
		OS::LogText(OS::ELogLevel_Info, "[%s] Connection closed, timeout: %d", m_sd->address.ToString().c_str(), m_timeout_flag);
		delete mp_mutex;
	}
	void Peer::think(bool packet_waiting) {
		NetIOCommResp io_resp;
		if (m_delete_flag) return;
		if (packet_waiting) {

			io_resp = ((FESL::Driver *)GetDriver())->getSSL_Socket_Interface()->streamRecv(m_sd, m_recv_buffer);

			int len = io_resp.comm_len;

			if ((io_resp.disconnect_flag || io_resp.error_flag)) {
				goto end;
			}

			FESL_HEADER header;
			m_recv_buffer.ReadBuffer(&header, sizeof(header));

			gettimeofday(&m_last_recv, NULL);

			size_t buf_len = len - sizeof(FESL_HEADER);

			if (buf_len < 0) {
				goto end;
			}

			OS::KVReader kv_data(std::string((const char *)m_recv_buffer.GetCursor(), buf_len), '=', '\n');
			char *type;
			for (int i = 0; i < sizeof(m_commands) / sizeof(CommandHandler); i++) {
				if (Peer::m_commands[i].type == htonl(header.type)) {
					if (Peer::m_commands[i].command.compare(kv_data.GetValue("TXN")) == 0) {
						type = (char *)&Peer::m_commands[i].type;
						OS::LogText(OS::ELogLevel_Info, "[%s] Got Command: %c%c%c%c %s", m_sd->address.ToString().c_str(), type[3], type[2], type[1], type[0], Peer::m_commands[i].command.c_str());
						(*this.*Peer::m_commands[i].mpFunc)(kv_data);
						return;
					}
				}
			}
			header.type = htonl(header.type);
			type = (char *)&header.type;
			OS::LogText(OS::ELogLevel_Info, "[%s] Got Unknown Command: %c%c%c%c %s", m_sd->address.ToString().c_str(), type[3], type[2], type[1], type[0], kv_data.GetValue("TXN").c_str());
		}

		end:
		send_ping();

		//check for timeout
		struct timeval current_time;
		gettimeofday(&current_time, NULL);
		if (current_time.tv_sec - m_last_recv.tv_sec > FESL_PING_TIME * 2) {
			Delete(true);
		}
		else if ((io_resp.disconnect_flag || io_resp.error_flag) && packet_waiting) {
			Delete();
		}
	}
	void Peer::send_ping() {
		//check for timeout
		struct timeval current_time;
		gettimeofday(&current_time, NULL);
		if(current_time.tv_sec - m_last_ping.tv_sec > FESL_PING_TIME) {
			gettimeofday(&m_last_ping, NULL);
			std::ostringstream s;
			s << "TXN=Ping\n";
			s << "TID=" << current_time.tv_sec << "\n";
			SendPacket(FESL_TYPE_FSYS, s.str());
		}
	}
	void Peer::SendPacket(FESL_COMMAND_TYPE type, std::string data, int force_sequence) {
		FESL_HEADER header;
		if (force_sequence == -1) {
			header.subtype = htonl(0x80000000 | m_sequence_id++);
		}
		else {
			header.subtype = htonl(0x80000000 | force_sequence);
		}
		
		header.type = htonl(type);
		header.len = htonl((u_long)data.length() + sizeof(header) + 1);

		OS::Buffer send_buf;
		send_buf.WriteBuffer((void *)&header, sizeof(header));
		send_buf.WriteNTS(data);

		NetIOCommResp io_resp = ((FESL::Driver *)GetDriver())->getSSL_Socket_Interface()->streamSend(m_sd, send_buf);

		if (io_resp.disconnect_flag || io_resp.error_flag)
			Delete();

	}
	void Peer::m_search_callback(OS::EProfileResponseType response_reason, std::vector<OS::Profile> results, std::map<int, OS::User> result_users, void *extra, INetPeer *peer) {
		((Peer *)peer)->mp_mutex->lock();
		((Peer *)peer)->m_profiles = results;
		((Peer *)peer)->mp_mutex->unlock();
		((Peer *)peer)->m_got_profiles = true;
		if (((Peer *)peer)->m_pending_subaccounts) {
			((Peer *)peer)->m_pending_subaccounts = false;
			((Peer *)peer)->send_subaccounts();
		}

		if (((Peer *)peer)->m_pending_nuget_personas) {
			((Peer *)peer)->m_pending_nuget_personas = false;
			((Peer *)peer)->send_personas();
		}
	}
	bool Peer::m_acct_get_account(OS::KVReader kv_list) {
		std::ostringstream s;
		s << "TXN=GetAccount\n";
		s << "parentalEmail=parents@ea.com\n";
		s << "countryCode=US\n";
		s << "countryDesc=\"United States of America\"\n";
		s << "thirdPartyMailFlag=0\n";
		s << "dobDay=" << (int)m_profile.birthday.GetDay() << "\n";
		s << "dobMonth=" << (int)m_profile.birthday.GetMonth() << "\n";
		s << "dobYear=" << (int)m_profile.birthday.GetYear() << "\n";
		s << "name=" << m_profile.nick << "\n";
		s << "email=" << m_user.email << "\n";
		s << "profileID=" << m_profile.id << "\n";
		s << "userId=" << m_user.id<< "\n";
		s << "zipCode=" << m_profile.zipcode << "\n";
		s << "gender=" << ((m_profile.sex == 0) ? 'M' : 'F') << "\n";
		s << "eaMailFlag=0\n";
		SendPacket(FESL_TYPE_ACCOUNT, s.str());
		return true;
	}
	bool Peer::m_subs_get_entitlement_by_bundle(OS::KVReader kv_list) {
		std::string kv_str = "TXN=GetEntitlementByBundle\n"
			"EntitlementByBundle.[]=0\n";
		SendPacket(FESL_TYPE_SUBS, kv_str);
		return true;
	}
	void Peer::send_subaccounts() {
		std::ostringstream s;
		mp_mutex->lock();
		std::vector<OS::Profile>::iterator it = m_profiles.begin();
		int i = 0;
		s << "TXN=GetSubAccounts\n";
		s << "subAccounts.[]=" << m_profiles.size() << "\n";
		while (it != m_profiles.end()) {
			OS::Profile profile = *it;
			s << "subAccounts." << i++ << "=\"" << profile.uniquenick << "\"\n";
			it++;
		}
		
		mp_mutex->unlock();
		SendPacket(FESL_TYPE_ACCOUNT, s.str());
	}
	void Peer::loginToSubAccount(std::string uniquenick) {
		std::ostringstream s;
		mp_mutex->lock();
		std::vector<OS::Profile>::iterator it = m_profiles.begin();
		while (it != m_profiles.end()) {
			OS::Profile profile = *it;
			if (profile.uniquenick.compare(uniquenick) == 0) {
				m_profile = profile;
				s << "TXN=LoginSubAccount\n";
				s << "lkey=" << m_session_key << "\n";
				s << "profileId=" << m_profile.id << "\n";
				s << "userId=" << m_user.id << "\n";
				SendPacket(FESL_TYPE_ACCOUNT, s.str());
				break;
			}
			it++;
		}
		mp_mutex->unlock();
	}
	bool Peer::m_acct_get_sub_accounts(OS::KVReader kv_list) {
		if (!m_got_profiles) {
			m_pending_subaccounts = true;
		}
		else {
			send_subaccounts();
		}
		return true;
	}
	bool Peer::m_dobj_get_object_inventory(OS::KVReader kv_list) {
		std::string kv_str = "TXN=GetObjectInventory\n"
			"ObjectInventory.[]=0\n";
		SendPacket(FESL_TYPE_DOBJ, kv_str);
		return true;
	}
	void Peer::send_memcheck(int type, int salt) {
		std::ostringstream s;
		s << "TXN=MemCheck\n";
		s << "memcheck.[]=0\n";
		s << "type=" << type << "\n";
		s << "salt=" << time(NULL) <<"\n";
		SendPacket(FESL_TYPE_FSYS, s.str(), 0);
	}
	bool Peer::m_acct_add_sub_account(OS::KVReader kv_list) {
		OS::ProfileSearchRequest request;
		std::string nick, oldnick;
		nick = kv_list.GetValue("name");

		request.user_search_details.id = m_user.id;
		//request.profile_search_details.id = m_profile.id;
		request.profile_search_details.namespaceid = FESL_PROFILE_NAMESPACEID;
		request.profile_search_details.nick = nick;
		request.profile_search_details.uniquenick = nick;
		request.extra = this;
		request.peer = this;
		request.peer->IncRef();
		request.type = OS::EProfileSearch_CreateProfile;
		request.callback = Peer::m_create_profile_callback;
		OS::m_profile_search_task_pool->AddRequest(request);
		
		return true;
	}
	void Peer::m_create_profile_callback(OS::EProfileResponseType response_reason, std::vector<OS::Profile> results, std::map<int, OS::User> result_users, void *extra, INetPeer *peer) {
		if (response_reason == OS::EProfileResponseType_Success && results.size() > 0) {
			((Peer *)peer)->mp_mutex->lock();
			((Peer *)peer)->m_profiles.push_back(results.front());
			((Peer *)peer)->mp_mutex->unlock();
			((Peer *)peer)->SendError(FESL_TYPE_ACCOUNT, FESL_ERROR_NO_ERROR, "AddSubAccount");
		} else {
			((Peer *)peer)->handle_profile_search_callback_error(response_reason, FESL_TYPE_ACCOUNT, "AddSubAccount");
		}
	}
	bool Peer::m_acct_disable_sub_account(OS::KVReader kv_list) {
		mp_mutex->lock();
		std::vector<OS::Profile>::iterator it = m_profiles.begin();
		while (it != m_profiles.end()) {
			OS::Profile profile = *it;
			if (profile.uniquenick.compare(kv_list.GetValue("name")) == 0) {
				OS::ProfileSearchRequest request;
				request.profile_search_details.id = profile.id;
				request.peer = this;
				request.extra = (void *)profile.id;
				request.peer->IncRef();
				request.type = OS::EProfileSearch_DeleteProfile;
				request.callback = Peer::m_delete_profile_callback;
				mp_mutex->unlock();
				OS::m_profile_search_task_pool->AddRequest(request);
				return true;
			}
			it++;
		}
		mp_mutex->unlock();
		return true;
	}
	void Peer::m_delete_profile_callback(OS::EProfileResponseType response_reason, std::vector<OS::Profile> results, std::map<int, OS::User> result_users, void *extra, INetPeer *peer) {
		if (response_reason == OS::EProfileResponseType_Success) {
			((Peer *)peer)->mp_mutex->lock();
			std::vector<OS::Profile>::iterator it = ((Peer *)peer)->m_profiles.begin();
			while (it != ((Peer *)peer)->m_profiles.end()) {
				OS::Profile profile = *it;
				if ((void *)profile.id == extra) {
					((Peer *)peer)->m_profiles.erase(it);
					break;
				}
				it++;
			}
			((Peer *)peer)->mp_mutex->unlock();

			std::ostringstream s;
			s << "TXN=DisableSubAccount\n";
			//((Peer *)peer)->SendPacket(FESL_TYPE_ACCOUNT, s.str());
			((Peer *)peer)->SendError(FESL_TYPE_ACCOUNT, (FESL_ERROR)0, "DisableSubAccount");
		}
		else {
			((Peer *)peer)->SendError(FESL_TYPE_ACCOUNT, (FESL_ERROR)FESL_ERROR_SYSTEM_ERROR, "DisableSubAccount");
		}
	}
	void Peer::m_create_auth_ticket(bool success, OS::User user, OS::Profile profile, OS::AuthData auth_data, void *extra, int operation_id, INetPeer *peer) {
		std::ostringstream s;
		if (success) {
			s << "TXN=GameSpyPreAuth\n";
			if(auth_data.hash_proof.length())
				s << "challenge=" << auth_data.hash_proof << "\n";
			s << "ticket=" << auth_data.session_key << "\n";
			((Peer *)peer)->SendPacket(FESL_TYPE_ACCOUNT, s.str());
		}
		else {
			((Peer *)peer)->SendError(FESL_TYPE_ACCOUNT, FESL_ERROR_AUTH_FAILURE, "GameSpyPreAuth");
		}
	}

	void Peer::SendCustomError(FESL_COMMAND_TYPE type, std::string TXN, std::string fieldName, std::string fieldError) {
		std::ostringstream s;
		s << "TXN=" << TXN << "\n";
		s << "errorContainer=[]\n";
		s << "errorCode=" << FESL_ERROR_CUSTOM << "\n";
		s << "errorContainer.0.fieldName=" << fieldName << "\n";
		s << "errorContainer.0.fieldError=" << fieldError << "\n";
		SendPacket(type, s.str());
	}
	void Peer::SendError(FESL_COMMAND_TYPE type, FESL_ERROR error, std::string TXN) {
		std::ostringstream s;
		s << "TXN=" << TXN << "\n";
		s << "errorContainer=[]\n";
		if (error == (FESL_ERROR)0) {
			s << "errorType=" << error << "\n";
		}
		s << "errorCode=" << error << "\n";
		SendPacket(type, s.str());
	}

	bool Peer::m_acct_get_country_list(OS::KVReader kv_list) {
		std::ostringstream s;
		s << "TXN=GetCountryList\n";
		s << "countryList.0.description=\"North America\"\n";
		s << "countryList.0.ISOCode=1\n";
		SendPacket(FESL_TYPE_ACCOUNT, s.str());
		return true;
	}
	bool Peer::m_acct_add_account(OS::KVReader kv_list) {
		/*
		Got EAMsg(166):
		TXN=AddAccount
		name=thisisatest
		password=123321
		email=chc@test.com
		DOBDay=11
		DOBMonth=11
		DOBYear=1966
		zipCode=111231
		countryCode=1
		eaMailFlag=1
		thirdPartyMailFlag=1
		*/
		OS::User user;
		OS::Profile profile;
		user.email = kv_list.GetValue("email");
		user.password = kv_list.GetValue("password");
		profile.uniquenick = kv_list.GetValue("name");
		profile.nick = profile.uniquenick;

		profile.birthday = OS::Date::Date(kv_list.GetValueInt("DOBYear"), kv_list.GetValueInt("DOBMonth"), kv_list.GetValueInt("DOBDay"));

		profile.namespaceid = FESL_ACCOUNT_NAMESPACEID;
		user.partnercode = OS_EA_PARTNER_CODE;
		OS::AuthTask::TryCreateUser_OrProfile(user, profile, true, m_newuser_cb, NULL, 0, this);
		
		return true;
	}
	void Peer::m_newuser_cb(bool success, OS::User user, OS::Profile profile, OS::AuthData auth_data, void *extra, int operation_id, INetPeer *peer) {
		FESL_ERROR err_code = FESL_ERROR_NO_ERROR;
		if (auth_data.response_code != -1 && auth_data.response_code != OS::LOGIN_RESPONSE_SUCCESS) {
			switch (auth_data.response_code) {
			case OS::CREATE_RESPONE_UNIQUENICK_IN_USE:
				err_code = FESL_ERROR_ACCOUNT_EXISTS;
				break;
			case OS::CREATE_RESPONSE_INVALID_EMAIL:
				((Peer *)peer)->SendCustomError(FESL_TYPE_ACCOUNT, "AddAccount", "Account.EmailAddress", "The specified email was invalid. Please change it and try again.");
				return;
			default:
				err_code = FESL_ERROR_SYSTEM_ERROR;
				break;
			}
			((Peer *)peer)->SendError(FESL_TYPE_ACCOUNT, (FESL_ERROR)err_code, "AddAccount");
		}
		else {
			std::ostringstream s;
			s << "TXN=AddAccount\n";
			s << "userId=" << user.id << "\n";
			s << "profileId=" << profile.id << "\n";
			((Peer *)peer)->SendPacket(FESL_TYPE_ACCOUNT, s.str());

		}
	}
	bool Peer::m_acct_update_account(OS::KVReader kv_list) {
		/*
		Got EAMsg(103):
		TXN=UpdateAccount
		email=chc@thmods.com
		parentalEmail=
		countryCode=1
		eaMailFlag=1
		thirdPartyMailFlag=0
		*/

		OS::Profile profile = m_profile;
		OS::User user = m_user;
		bool send_userupdate = false; //, send_profileupdate = false;

		if (kv_list.GetValue("email").compare(m_user.email) == 0) {
			user.email = kv_list.GetValue("email");
			send_userupdate = true;
		}
		/*
		OS::ProfileSearchRequest request;
		request.profile_search_details = m_profile;
		request.extra = NULL;
		request.peer = this;
		request.peer->IncRef();
		request.type = OS::EProfileSearch_UpdateProfile;
		request.callback = Peer::m_update_profile_callback;
		OS::m_profile_search_task_pool->AddRequest(request);
		*/

		if (send_userupdate) {
			OS::UserSearchRequest user_request;
			user_request.search_params = user;
			user_request.type = OS::EUserRequestType_Update;
			user_request.extra = NULL;
			user_request.peer = this;
			user_request.peer->IncRef();
			user_request.callback = Peer::m_update_user_callback;
			OS::m_user_search_task_pool->AddRequest(user_request);
		}
		return true;
	}
	void Peer::m_update_user_callback(OS::EUserResponseType response_reason, std::vector<OS::User> results, void *extra, INetPeer *peer) {
		std::ostringstream s;
		s << "TXN=UpdateAccount\n";
		if (response_reason == OS::EProfileResponseType_Success) {
			((Peer *)peer)->SendPacket(FESL_TYPE_ACCOUNT, s.str());
		}
		else {
			((Peer *)peer)->SendError(FESL_TYPE_ACCOUNT, FESL_ERROR_SYSTEM_ERROR, "UpdateAccount");
		}
	}
	bool Peer::m_acct_gettos_handler(OS::KVReader kv_list) {
		std::ostringstream s;
		s << "TXN=GetTos\n";
		s << "tos=\"Hi\"\n";
		SendPacket(FESL_TYPE_FSYS, s.str());
		return true;
	}
	void Peer::Delete(bool timeout) {
		m_timeout_flag = timeout;
		m_delete_flag = true;
	}
	bool Peer::GetAuthCredentials(OS::User &user, OS::Profile &profile) {
		if(m_user.id) {
			user = m_user;
			profile = m_profile;
			return true;
		}
		return false;
	}
	void Peer::DuplicateLoginExit() {
		std::string kv_str = "TXN=Goodbye\n";
		SendPacket(FESL_TYPE_FSYS, kv_str);
		Delete();
	}
}