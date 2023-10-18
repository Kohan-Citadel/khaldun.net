#include "UTPeer.h"
#include "UTServer.h"
#include "UTDriver.h"

#include "../tasks/tasks.h"
namespace UT {
	Server::Server() : INetServer(){
		uv_loop_set_data(uv_default_loop(), this);
		init();
	}
	Server::~Server() {
	}
	void Server::init() {
		MM::InitTasks();
	}
	void Server::tick() {
		std::vector<INetDriver *>::iterator it = m_net_drivers.begin();
		while (it != m_net_drivers.end()) {
			INetDriver *driver = *it;
			driver->think(false);
			it++;
		}
		NetworkTick();
	}
	void Server::shutdown() {

	}

    void Server::doInternalLoadGameData(redisContext *ctx) {
        std::vector<INetDriver *>::iterator it = m_net_drivers.begin();
        while (it != m_net_drivers.end()) {
            UT::Driver *driver = (UT::Driver *)*it;
            std::vector<UT::Config *> cfg_list = driver->GetConfig();
            std::vector<UT::Config *>::iterator it2 = cfg_list.begin();
            while(it2 != cfg_list.end()) {
                UT::Config *cfg = *it2;
                cfg->game_data = OS::GetGameByID(cfg->gameid, ctx);
                it2++;
            }
            it++;
        }
    }
}
