#include <OS/SharedTasks/tasks.h>
#include "../AuthTasks.h"

namespace TaskShared {
	bool PerformAuth_UniqueNick_Password(AuthRequest request, TaskThreadData *thread_data) {
		curl_data recv_data;
		//build json object
		json_t *send_obj = json_object(), *profile_obj = json_object();

		json_object_set_new(profile_obj, "uniquenick", json_string(request.profile.uniquenick.c_str()));

		if(request.profile.namespaceid != -1)
			json_object_set_new(profile_obj, "namespaceid", json_integer(request.profile.namespaceid));

		json_object_set_new(send_obj, "profileLookup", profile_obj);

		json_object_set_new(send_obj, "password", json_string(request.user.password.c_str()));

		char *json_dump = json_dumps(send_obj, 0);

		CURL *curl = curl_easy_init();
		CURLcode res;
		OS::Profile profile;
		OS::User user;
		user.id = 0;
		profile.id = 0;
		TaskShared::AuthData auth_data;

		auth_data.response_code = TaskShared::LOGIN_RESPONSE_SERVER_ERROR;
		bool success = false;
		if (curl) {

			AuthReq_InitCurl(curl, json_dump, (void *)&recv_data, request);

			res = curl_easy_perform(curl);

			if (res == CURLE_OK) {
				json_t *json_data = json_loads(recv_data.buffer.c_str(), 0, NULL);
				if (json_data) {
					json_t *error_obj = json_object_get(json_data, "error");
					json_t *success_obj = json_object_get(json_data, "success");
					if (error_obj) {
						Handle_AuthWebError(auth_data, error_obj);
					}
					else if (success_obj == json_true()) {
						success = true;
						json_t *session_key_json = json_object_get(json_data, "session_key");
						if (session_key_json) {
							auth_data.session_key = json_string_value(session_key_json);
						}

						session_key_json = json_object_get(json_data, "profile");
						if (session_key_json) {
							profile = OS::LoadProfileFromJson(session_key_json);
						}

						session_key_json = json_object_get(json_data, "user");
						if (session_key_json) {
							user = OS::LoadUserFromJson(session_key_json);
						}
					}
					json_decref(json_data);
				}
			}
			curl_easy_cleanup(curl);
		}
		request.callback(success, user, profile, auth_data, request.extra, request.peer);
		if (json_dump) {
			free((void *)json_dump);
		}
		json_decref(send_obj);
		return false;
	}
}