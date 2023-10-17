#include <tasks/tasks.h>
#include <server/SBServer.h>
#include <server/SBDriver.h>
namespace MM {
    bool Handle_ServerEventMsg(std::string message) {
		std::vector<std::string> vec = OS::KeyStringToVector(message);
		if (vec.size() < 2) return false;
		std::string msg_type = vec.at(0);
		std::string server_key = vec.at(1);

		TaskThreadData thread_data;
		thread_data.mp_redis_connection = getThreadLocalRedisContext();
		SBServer *server = (SBServer *)uv_loop_get_data(uv_default_loop());


		MM::Server *gameserver = GetServerByKey(&thread_data, server_key, msg_type.compare("del") == 0);
		if(!server) return false;

		MM::Server server_cpy = *gameserver;
		delete gameserver;

		if(msg_type.compare("del") == 0) {
			server->OnDeleteServer(server_cpy);
		} else if(msg_type.compare("new") == 0) {
			server->OnNewServer(server_cpy);
		} else if(msg_type.compare("update") == 0) {
			server->OnUpdateServer(server_cpy);
		}
        return true;
    }
}