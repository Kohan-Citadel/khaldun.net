#ifndef _TASKS_SHARED_H
#define _TASKS_SHARED_H
#include "WebError.h"
#include "Auth/AuthTasks.h"
#include "Account/UserTasks.h"
#include "Account/ProfileTasks.h"
#include "CDKey/CDKeyTasks.h"
namespace TaskShared {
	struct curl_data {
		std::string buffer;
	};

    TaskScheduler<AuthRequest, TaskThreadData> *InitAuthTasks(INetServer *server);
    TaskScheduler<UserRequest, TaskThreadData> *InitUserTasks(INetServer *server);
    TaskScheduler<ProfileRequest, TaskThreadData> *InitProfileTasks(INetServer *server);
    TaskScheduler<CDKeyRequest, TaskThreadData> *InitCDKeyTasks(INetServer *server);
    size_t curl_callback(void *contents, size_t size, size_t nmemb, void *userp);
}
#endif //_TASKS_SHARED_H