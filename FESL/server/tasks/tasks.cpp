#include "tasks.h"
namespace FESL {
	const char *auth_channel_exchange = "presence.core";
	const char *auth_routingkey = "presence.buddies";
	TaskScheduler<FESLRequest, TaskThreadData> *InitTasks(INetServer *server) {
		TaskScheduler<FESLRequest, TaskThreadData> *scheduler = new TaskScheduler<FESLRequest, TaskThreadData>(OS::g_numAsync, server);
		scheduler->AddRequestListener("openspy.core", "auth.events", Handle_AuthEvent);
		scheduler->DeclareReady();
		return scheduler;
	}

	bool Handle_AuthEvent(TaskThreadData *thread_data, std::string message) {
		FESL::Server *server = (FESL::Server *)thread_data->server;
		OS::KVReader reader = OS::KVReader(message);
		std::string msg_type = reader.GetValue("type");
		if (msg_type.compare("auth_event") == 0) {
			std::string appName = reader.GetValue("app_name");
			if (appName.compare("FESL") == 0) {
				int profileid = reader.GetValueInt("profileid");
				int userid = reader.GetValueInt("userid");
				std::string session_key = reader.GetValue("session_key");
				server->OnUserAuth(session_key, userid, profileid);
			}
		}
		return true;
	}
}