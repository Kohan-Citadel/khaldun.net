#ifndef _NN_TASKS_H
#define _NN_TASKS_H
#include <string>

#include <server/structs.h>

#include <OS/Task/TaskScheduler.h>

#include <OS/MessageQueue/MQInterface.h>

#define NN_REDIS_EXPIRE_TIME 500

NN::ConnectionSummary LoadConnectionSummary(std::string redis_key);
namespace NN {
    class Server;
    class Peer;
    enum ENNRequestType {
        ENNRequestType_SubmitJson
    };
    class NNRequestData {
        public:
            std::string send_string;
			TaskSchedulerRequestType type;
    };
    extern const char *nn_channel_exchange;
    extern const char *nn_channel_routingkey;

    bool PerformSubmitJson(NNRequestData, TaskThreadData *);

	void PerformUVWorkRequest(uv_work_t *req);
	void PerformUVWorkRequestCleanup(uv_work_t *req, int status);

    bool Handle_HandleRecvMessage(TaskThreadData *, std::string message);
    void InitTasks();

    amqp_connection_state_t getThreadLocalAmqpConnection();
}
#endif // _NN_TASKS_H
