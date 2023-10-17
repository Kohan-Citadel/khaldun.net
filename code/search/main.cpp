#include <stdio.h>
#include <map>
#include <string>
#include <sstream>
#include <OS/Net/NetServer.h>
#include <OS/Config/AppConfig.h>
#include "server/SMServer.h"
#include "server/SMDriver.h"
INetServer *g_gameserver = NULL;

void idle_handler(uv_idle_t* handle) {
	g_gameserver->tick();
}


int main() {
	uv_loop_t *loop = uv_default_loop();
	uv_idle_t idler;

	uv_idle_init(uv_default_loop(), &idler);
    uv_idle_start(&idler, idle_handler);


	OS::Init("SM", NULL);

	g_gameserver = new SM::Server();


	char address_buff[256];
	char port_buff[16];
	size_t temp_env_sz = sizeof(address_buff);

	if(uv_os_getenv("OPENSPY_SM_BIND_ADDR", (char *)&address_buff, &temp_env_sz) != UV_ENOENT) {
		temp_env_sz = sizeof(port_buff);
		uv_os_getenv("OPENSPY_SM_BIND_PORT", (char *)&port_buff, &temp_env_sz);
		uint16_t port = atoi(port_buff);

		SM::Driver *driver = new SM::Driver(g_gameserver, address_buff, port, false);

		OS::LogText(OS::ELogLevel_Info, "Adding SM Driver: %s:%d\n", address_buff, port);
		g_gameserver->addNetworkDriver(driver);
	}


	g_gameserver->init();

    uv_run(loop, UV_RUN_DEFAULT);

    uv_loop_close(loop);
	
    delete g_gameserver;

    OS::Shutdown();
    return 0;
}