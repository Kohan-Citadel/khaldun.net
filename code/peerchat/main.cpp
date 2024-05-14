#include <stdio.h>
#include <map>
#include <string>
#include <sstream>
#include <OS/Net/NetServer.h>
#include "server/Server.h"
#include "server/Driver.h"
#include "tasks/tasks.h"
INetServer *g_gameserver = NULL;

void tick_handler(uv_timer_t* handle) {
	g_gameserver->tick();
}

int main() {
	uv_loop_t *loop = uv_default_loop();
	uv_timer_t tick_timer;

	uv_timer_init(uv_default_loop(), &tick_timer);

	OS::Init("peerchat");

	g_gameserver = new Peerchat::Server();


	char address_buff[256];
	char port_buff[16];
	char server_name_buff[256];
	size_t temp_env_sz = sizeof(address_buff);

	if(uv_os_getenv("OPENSPY_PEERCHAT_BIND_ADDR", (char *)&address_buff, &temp_env_sz) != UV_ENOENT) {
		temp_env_sz = sizeof(port_buff);

		uint16_t port = 6667;
		if(uv_os_getenv("OPENSPY_PEERCHAT_BIND_PORT", (char *)&port_buff, &temp_env_sz) != UV_ENOENT) {
			port = atoi(port_buff);
		}

		std::string server_name;
		temp_env_sz = sizeof(server_name_buff);
		if(uv_os_getenv("OPENSPY_PEERCHAT_SERVER_NAME", (char *)&server_name_buff, &temp_env_sz) != 0) {
			server_name = "Matrix";
		} else {
			server_name = std::string(server_name_buff);
		}

		void *ssl_ctx = OS::GetSSLContext();
		Peerchat::Driver *driver = new Peerchat::Driver(g_gameserver, server_name, address_buff, port, ssl_ctx);

		OS::LogText(OS::ELogLevel_Info, "Adding Peerchat Driver: %s:%d\n", address_buff, port);
		g_gameserver->addNetworkDriver(driver);
	} else {
		OS::LogText(OS::ELogLevel_Warning, "Missing Peerchat bind address environment variable");
	}

	Peerchat::InitTasks();
	uv_timer_start(&tick_timer, tick_handler, 0, 250);
	uv_run(loop, UV_RUN_DEFAULT);

	uv_loop_close(loop);

	delete g_gameserver;

	OS::Shutdown();
	return 0;
}
