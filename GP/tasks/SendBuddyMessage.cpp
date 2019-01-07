#include "tasks.h"
namespace GP {
    bool Perform_SendBuddyMessage(GPBackendRedisRequest request, TaskThreadData *thread_data) {
		TaskShared::curl_data recv_data;
		//build json object
		json_t *send_obj = json_object(), *to_obj = json_object(), *from_obj = json_object(), *lookup_obj = json_object();

		json_object_set_new(to_obj, "id", json_integer(request.ToFromData.to_profileid));
		json_object_set_new(from_obj, "id", json_integer(request.ToFromData.from_profileid));

		json_object_set(send_obj, "message", json_string(request.BuddyMessage.message.c_str()));

		json_object_set(lookup_obj, "sourceProfile", from_obj);
		json_object_set(lookup_obj, "targetProfile", to_obj);
		json_object_set(send_obj, "lookup", lookup_obj);


		char *json_dump = json_dumps(send_obj, 0);

		CURL *curl = curl_easy_init();
		CURLcode res;
		OS::Profile profile;
		OS::User user;
		user.id = 0;
		profile.id = 0;
		TaskShared::AuthData auth_data;
		bool success = false;

		auth_data.response_code = TaskShared::LOGIN_RESPONSE_SERVER_ERROR;


		if (curl) {
			GPReq_InitCurl(curl, json_dump, (void *)&recv_data, request);

			res = curl_easy_perform(curl);

			if (res == CURLE_OK) {
				//TODO: error handling
			}
			curl_easy_cleanup(curl);
		}
		if (json_dump) {
			free((void *)json_dump);
		}
		json_decref(send_obj);
		return true;
    }
}