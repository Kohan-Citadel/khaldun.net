#include <OS/OpenSpy.h>
#include <OS/Net/NetPeer.h>
#include <OS/SharedTasks/tasks.h>
#include "CDKeyTasks.h"
namespace TaskShared {
    TaskScheduler<CDKeyRequest, TaskThreadData> *InitCDKeyTasks(INetServer *server) {
            TaskScheduler<CDKeyRequest, TaskThreadData> *scheduler = new TaskScheduler<CDKeyRequest, TaskThreadData>(OS::g_numAsync, server);
            scheduler->SetThreadDataFactory(TaskScheduler<CDKeyRequest, TaskThreadData>::DefaultThreadDataFactory);

            scheduler->AddRequestHandler(ECDKeyType_AssociateToProfile, PerformCDKey_AssociateToProfile);
            scheduler->AddRequestHandler(ECDKeyType_GetProfileByCDKey, PerformCDKey_GetProfileByCDKey);
            scheduler->AddRequestHandler(ECDKeyType_TestCDKeyValid, PerformCDKey_TestCDKeyValid);
			scheduler->DeclareReady();
            return scheduler;
    }
    void CDKeyReq_InitCurl(void *curl, char *post_data, void *write_data, ProfileRequest request) {
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
            case ECDKeyType_AssociateToProfile:
                url += "/v1/CDKey/AssociateCDKeyToProfile";
                break;
            case ECDKeyType_GetProfileByCDKey:
                url += "/v1/CDKey/GetProfileByCDKey";
                break;
            case ECDKeyType_TestCDKeyValid:
                url += "/v1/CDKey/TestCDKeyValid";
                break;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);

        /* set default user agent */
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "OSCDKey");

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