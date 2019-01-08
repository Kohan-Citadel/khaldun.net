#include <OS/OpenSpy.h>
#include <OS/Search/Profile.h>

#include <curl/curl.h>
#include <jansson.h>

#include <ctype.h>

namespace OS {
	struct curl_data {
		std::string buffer;
	};
	/* callback for curl fetch */
	size_t ProfileSearchTask::curl_callback(void *contents, size_t size, size_t nmemb, void *userp) {
		if (!contents) {
			return 0;
		}
		size_t realsize = size * nmemb;                             /* calculate buffer size */
		curl_data *data = (curl_data *)userp;
		data->buffer += OS::strip_whitespace((const char *)contents).c_str();;
		return realsize;
	}
	void ProfileSearchTask::PerformSearch(ProfileSearchRequest request) {
		curl_data recv_data;
		recv_data.buffer = "";
		std::vector<OS::Profile> results;
		std::map<int, OS::User> users_map;
		//build json object

		json_t *profile_obj = json_object();
		json_t *send_obj = profile_obj;
		json_t *user_obj = json_object();
		if (request.profile_search_details.id != 0)
			json_object_set_new(profile_obj, "id", json_integer(request.profile_search_details.id));

		json_object_set_new(profile_obj, "userid", json_integer(request.profile_search_details.userid));

		//user parameters
		if (request.user_search_details.email.length()) {
			json_object_set_new(user_obj, "email", json_string(request.user_search_details.email.c_str()));
		}			

		if (request.user_search_details.id != 0) {
			//json_object_set_new(profile_obj, "userid", json_integer(request.user_search_details.id));
			json_object_set_new(user_obj, "id", json_integer(request.user_search_details.id));
			
		}


		if(request.user_search_details.partnercode != -1) {
			json_object_set_new(user_obj, "partnercode", json_integer(request.user_search_details.partnercode));
		}



		//profile parameters
		if (request.profile_search_details.nick.length())
			json_object_set_new(profile_obj, "nick", json_string(request.profile_search_details.nick.c_str()));

		if (request.profile_search_details.uniquenick.length())
			json_object_set_new(profile_obj, "uniquenick", json_string(request.profile_search_details.uniquenick.c_str()));

		if (request.profile_search_details.firstname.length())
			json_object_set_new(profile_obj, "firstname", json_string(request.profile_search_details.firstname.c_str()));

		if (request.profile_search_details.lastname.length())
			json_object_set_new(profile_obj, "lastname", json_string(request.profile_search_details.lastname.c_str()));

		if (request.profile_search_details.icquin)
			json_object_set_new(profile_obj, "icquin", json_integer(request.profile_search_details.icquin));

		if (request.profile_search_details.zipcode)
			json_object_set_new(profile_obj, "zipcode", json_integer(request.profile_search_details.zipcode));


		if(request.profile_search_details.sex != -1)
			json_object_set_new(profile_obj, "sex", json_integer(request.profile_search_details.sex));

		if(request.profile_search_details.pic != 0)
			json_object_set_new(profile_obj, "pic", json_integer(request.profile_search_details.pic));
		if (request.profile_search_details.ooc != 0)
			json_object_set_new(profile_obj, "ooc", json_integer(request.profile_search_details.ooc));
		if (request.profile_search_details.ind!= 0)
			json_object_set_new(profile_obj, "ind", json_integer(request.profile_search_details.ind));
		if (request.profile_search_details.mar!= 0)
			json_object_set_new(profile_obj, "mar", json_integer(request.profile_search_details.mar));
		if (request.profile_search_details.chc != 0)
			json_object_set_new(profile_obj, "chc", json_integer(request.profile_search_details.chc));
		if (request.profile_search_details.i1 != 0)
			json_object_set_new(profile_obj, "i1", json_integer(request.profile_search_details.i1));

		if (request.profile_search_details.birthday.GetYear() != 0)
			json_object_set_new(profile_obj, "birthday", request.profile_search_details.birthday.GetJson());

		if(request.profile_search_details.lon)
			json_object_set_new(profile_obj, "lon", json_real(request.profile_search_details.lon));
		if(request.profile_search_details.lat)
			json_object_set_new(profile_obj, "lat", json_real(request.profile_search_details.lat));


		if(request.profile_search_details.namespaceid != -1)
			json_object_set_new(profile_obj, "namespaceid", json_integer(request.profile_search_details.namespaceid));


		if (request.namespaceids.size()) {
			json_t *namespaceids_json = json_array();

			//json_array_append_new(v_array, json_real(v));
			std::vector<int>::iterator it = request.namespaceids.begin();
			while (it != request.namespaceids.end()) {
				int v = *it;
				json_array_append_new(namespaceids_json, json_integer(v));
				it++;
			}

			json_object_set_new(profile_obj, "namespaceids", namespaceids_json);

		}
		//json_object_set(send_obj, "profile", profile_obj);
		//json_object_set(send_obj, "user", user_obj);


		if (request.target_profileids.size()) {
			json_t *profileids_json = json_array();

			std::vector<int>::iterator it = request.target_profileids.begin();
			while (it != request.target_profileids.end()) {
				int v = *it;
				json_array_append_new(profileids_json, json_integer(v));
				it++;
			}

			json_object_set_new(send_obj, "target_profileids", profileids_json);
		}


		char *json_data = json_dumps(send_obj, 0);

		CURL *curl = curl_easy_init();
		CURLcode res;
		EProfileResponseType error = EProfileResponseType_GenericError;
		if (curl) {

			ProfileReq_InitCurl(curl, json_data, (void *)&recv_data, request);
			res = curl_easy_perform(curl);
			if (res == CURLE_OK) {
				json_t *json_data = NULL;
				
				json_data = json_loads(recv_data.buffer.c_str(), 0, NULL);

				if (json_data) {
					error = EProfileResponseType_Success;
					json_t *error_obj = json_object_get(json_data, "error");

					if(error_obj) {
						error = Handle_ProfileWebError(request, error_obj);
					} else if (json_is_array(json_data)) {
						size_t num_profiles = json_array_size(json_data);
						for (size_t i = 0; i < num_profiles; i++) {
							json_t *profile_obj = json_array_get(json_data, i);
							OS::Profile profile = OS::LoadProfileFromJson(profile_obj);
							if (users_map.find(profile.userid) == users_map.end()) {
								json_t *user_obj = json_object_get(profile_obj, "user");
								users_map[profile.userid] = OS::LoadUserFromJson(user_obj);
							}
							results.push_back(profile);
						}
					}
					else {
						//check for single profile
						OS::Profile profile = OS::LoadProfileFromJson(json_data);
						results.push_back(profile);
					}
					json_decref(json_data);
				}
				else {
					error = EProfileResponseType_GenericError;
				}
			}
			curl_easy_cleanup(curl);
		}

		if (json_data) {
			free((void *)json_data);
		}
		if (send_obj)
			json_decref(send_obj);


		request.callback(error, results, users_map, request.extra, request.peer);
	}

	EProfileResponseType ProfileSearchTask::Handle_ProfileWebError(ProfileSearchRequest req, json_t *error_obj) {
		std::string error_class, error_name, param_name;
		json_t *item = json_object_get(error_obj, "class");
		if (!item) goto end_error;
		error_class = json_string_value(item);

		item = json_object_get(error_obj, "name");
		if (!item) goto end_error;
		error_name = json_string_value(item);

		item = json_object_get(error_obj, "param");
		if (item) {
			param_name = json_string_value(item);
		}

		if (error_class.compare("common") == 0) {
			if (error_name.compare("MissingParam") == 0 || error_name.compare("InvalidMode") == 0) {
				return EProfileResponseType_GenericError;
			}
			else if (error_name.compare("InvalidParam") == 0) {
				if (param_name.compare("UniqueNick") == 0) {
					return EProfileResponseType_UniqueNick_Invalid;
				}
			}
		}
		else if (error_class.compare("auth") == 0) {
			return EProfileResponseType_GenericError;
		}
		else if (error_class.compare("profile") == 0) {
			if (error_name.compare("UniqueNickInUse") == 0) {
				return EProfileResponseType_UniqueNick_InUse;
			}
		}
		end_error:
		return EProfileResponseType_GenericError;

	}
	void *ProfileSearchTask::TaskThread(CThread *thread) {
		ProfileSearchTask *task = (ProfileSearchTask *)thread->getParams();
		
		while (thread->isRunning() && (!task->m_request_list.empty() || task->mp_thread_poller->wait(PROFILE_TASK_WAIT_MAX_TIME)) && thread->isRunning()) {
			task->mp_mutex->lock();
			while (!task->m_request_list.empty()) {
				ProfileSearchRequest task_params = task->m_request_list.front();
				task->mp_mutex->unlock();
				PerformSearch(task_params);

				if (task_params.peer)
					task_params.peer->DecRef();

				task->mp_mutex->lock();
				task->m_request_list.pop();
			}
			task->mp_mutex->unlock();
		}
		return NULL;
	}
	ProfileSearchTask::ProfileSearchTask(int thread_index) : Task<ProfileSearchRequest>() {
		mp_mutex = OS::CreateMutex();
		mp_thread = OS::CreateThread(ProfileSearchTask::TaskThread, this, true);

	}
	ProfileSearchTask::~ProfileSearchTask() {
		mp_thread->SignalExit(true, mp_thread_poller);
		delete mp_mutex;
		delete mp_thread;
	}
	void ProfileSearchTask::ProfileReq_InitCurl(void *curl, char *post_data, void *write_data, ProfileSearchRequest request) {
		struct curl_slist *chunk = NULL;
		std::string apiKey = "APIKey: " + std::string(OS::g_webServicesAPIKey);
		chunk = curl_slist_append(chunk, apiKey.c_str());
		chunk = curl_slist_append(chunk, "Content-Type: application/json");
		chunk = curl_slist_append(chunk, std::string(std::string("X-OpenSpy-App: ") + OS::g_appName).c_str());
		if (request.peer) {
			chunk = curl_slist_append(chunk, std::string(std::string("X-OpenSpy-Peer-Address: ") + request.peer->getAddress().ToString()).c_str());
		}
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

		std::string url = OS::g_webServicesURL;
		switch (request.type) {
			case EProfileSearch_Profiles:
				url += "/v1/Profile/lookup";
				break;
			case EProfileSearch_CreateProfile:
			case EProfileSearch_DeleteProfile:
			case EProfileSearch_UpdateProfile:
				url += "/v1/Profile";
				break;
			case EProfileSearch_Blocks:
				url += "/v1/Presence/Block";
				break;
			case EProfileSearch_Buddies:
			case EProfileSearch_Buddies_Reverse:
				url += "/v1/Presence/Buddy";
				break;
		}
		switch (request.type) {
			case EProfileSearch_CreateProfile:
				curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
				break;
			case EProfileSearch_DeleteProfile:
				curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
				break;
		}
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);

		/* set default user agent */
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "OSSearchProfile");

		/* set timeout */
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);

		/* enable location redirects */
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);

		/* set maximum allowed redirects */
		curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 1);

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_callback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, write_data);

		/* Close socket after one use */
		curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1);
	}
}