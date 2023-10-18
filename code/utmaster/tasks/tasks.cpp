#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/tcp_socket.h>
#include "tasks.h"


namespace MM {
	const char *mm_channel_exchange = "openspy.master";
	const char *mm_server_event_routingkey = "server.event";
    const char *mp_pk_name = "QRID";

	uv_thread_t m_amqp_consumer_thread;

	uv_work_t internal_game_load_work;


    void InitTasks() {
		
		MM::MMWorkData *work_data = new MM::MMWorkData();
		work_data->request.type = UTMasterRequestType_InternalLoadGamename;

		uv_handle_set_data((uv_handle_t*) &internal_game_load_work, work_data);

		uv_queue_work(uv_default_loop(), &internal_game_load_work, MM::PerformUVWorkRequest, MM::PerformUVWorkRequestCleanup);
    }

    void selectQRRedisDB(TaskThreadData *thread_data) {
        void *reply;
        reply = redisCommand(thread_data->mp_redis_connection, "SELECT %d", OS::ERedisDB_QR);
        if(reply) {
            freeReplyObject(reply);
        }
    }

	void PerformUVWorkRequest(uv_work_t *req) {
		TaskThreadData temp_data;
		temp_data.mp_redis_connection = TaskShared::getThreadLocalRedisContext();
		MM::MMWorkData *work_data = (MM::MMWorkData *) uv_handle_get_data((uv_handle_t*) req);
		
		switch(work_data->request.type) {
			case UTMasterRequestType_Heartbeat:
				PerformHeartbeat(work_data->request, &temp_data);
			break;
			case UTMasterRequestType_ListServers:
				PerformListServers(work_data->request, &temp_data);
			break;
			case UTMasterRequestType_DeleteServer:
				PerformDeleteServer(work_data->request, &temp_data);
			break;
			case UTMasterRequestType_InternalLoadGamename:
				PerformInternalLoadGameData(work_data->request, &temp_data);
			break;
		}
	}
	void PerformUVWorkRequestCleanup(uv_work_t *req, int status) {
		MM::MMWorkData *work_data = (MM::MMWorkData *) uv_handle_get_data((uv_handle_t*) req);
		delete work_data;
		free((void *)req);
	}
}
