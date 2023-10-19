#include <stdio.h>
#include <map>
#include <string>
#include <sstream>
#include <OS/Net/NetServer.h>
#include "server/GSServer.h"
#include "server/GSDriver.h"
INetServer *g_gameserver = NULL;

void tick_handler(uv_timer_t* handle) {
	g_gameserver->tick();
}

int main() {
	uv_loop_t *loop = uv_default_loop();
	uv_timer_t tick_timer;

	uv_timer_init(uv_default_loop(), &tick_timer);
    uv_timer_start(&tick_timer, tick_handler, 0, 250);

	OS::Init("GS");
	g_gameserver = new GS::Server();

	char address_buff[256];
	char port_buff[16];
	size_t temp_env_sz = sizeof(address_buff);

	if(uv_os_getenv("OPENSPY_GSTATS_BIND_ADDR", (char *)&address_buff, &temp_env_sz) != UV_ENOENT) {
		temp_env_sz = sizeof(port_buff);
		uv_os_getenv("OPENSPY_GSTATS_BIND_PORT", (char *)&port_buff, &temp_env_sz);
		uint16_t port = atoi(port_buff);

		GS::Driver *driver = new GS::Driver(g_gameserver, address_buff, atoi(port_buff));

		OS::LogText(OS::ELogLevel_Info, "Adding Driver: %s:%d\n", address_buff, port);
		g_gameserver->addNetworkDriver(driver);
	}

	g_gameserver->init();

    uv_run(loop, UV_RUN_DEFAULT);

    uv_loop_close(loop);

    delete g_gameserver;

    OS::Shutdown();
    
    return 0;
}