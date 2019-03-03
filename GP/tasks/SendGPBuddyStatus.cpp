#include "tasks.h"
namespace GP {
    bool Perform_GetBuddyStatus(GPBackendRedisRequest request, TaskThreadData *thread_data) {
		TaskShared::curl_data recv_data;
		//build json object
		json_t *send_obj = json_object();

		json_object_set_new(send_obj, "id", json_integer(request.peer->GetProfileID()));


		char *json_dump = json_dumps(send_obj, 0);

		CURL *curl = curl_easy_init();
		CURLcode res;
		OS::Profile profile;
		OS::User user;
		user.id = 0;
		profile.id = 0;
		TaskShared::AuthData auth_data;
		bool success = false;

		if (curl) {
			struct curl_slist *chunk = NULL;
			GPReq_InitCurl(curl, json_dump, (void *)&recv_data, request, &chunk);

			res = curl_easy_perform(curl);

			if (res == CURLE_OK) {
			}
			curl_slist_free_all(chunk);
			curl_easy_cleanup(curl);
		}

		//no callback yet..

		if (json_dump) {
			free((void *)json_dump);
		}
		json_decref(send_obj);

		if (request.peer)
			request.peer->DecRef();
		return true;
    }
}