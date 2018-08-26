#include <stdio.h>
#include <map>
#include <string>
#include <sstream>
#include <OS/Net/NetServer.h>
#include <openssl/ssl.h>
#include "server/FESLServer.h"
#include "server/FESLDriver.h"
#include <OS/StringCrypter.h>
INetServer *g_gameserver = NULL;
bool g_running = true;

void shutdown();

void on_exit(void) {
    shutdown();
}

void sig_handler(int signo)
{
    shutdown();
}

std::string get_file_contents(std::string path) {
	std::string ret;
	FILE *fd = fopen(path.c_str(),"r");
	if(fd) {
		fseek(fd,0,SEEK_END);
		int len = ftell(fd);
		fseek(fd,0,SEEK_SET);

		char *str_data = malloc(len+1);
		fread(str_data, len, 1, fd);
		str_data[len] = 0;
		ret = str_data;
		free((void *)str_data);
	}
	fclose(fd);
	return ret;
}

SSLNetIOIFace::ESSL_Type getSSLVersion(configVar *driver_arr) {
	std::string ssl_version = OS::g_config->getArrayString(driver_arr, "ssl_version");

	if (ssl_version.compare("SSLv2") == 0) {
		return SSLNetIOIFace::ESSL_SSLv2;
	}
	if (ssl_version.compare("SSLv23") == 0) {
		return SSLNetIOIFace::ESSL_SSLv23;
	}
	if (ssl_version.compare("SSLv3") == 0) {
		return SSLNetIOIFace::ESSL_SSLv3;
	}
	if (ssl_version.compare("TLS1.0") == 0) {
		return SSLNetIOIFace::ESSL_TLS10;
	}
	if (ssl_version.compare("TLS1.1") == 0) {
		return SSLNetIOIFace::ESSL_TLS11;
	}
	if (ssl_version.compare("TLS1.2") == 0) {
		return SSLNetIOIFace::ESSL_TLS12;
	}
	return SSLNetIOIFace::ESSL_None;
}

#include <curl/curl.h>
int main() {
	SSL_library_init();
	#ifndef _WIN32
		signal(SIGPIPE, SIG_IGN); // due to openssl SSL_write, we must block broken pipes here
		signal(SIGINT, sig_handler);
		signal(SIGTERM, sig_handler);
	#else
		WSADATA wsdata;
		WSAStartup(MAKEWORD(1, 0), &wsdata);
	#endif

	OS::Init("FESL", "openspy.cfg");

	g_gameserver = new FESL::Server();
	configVar *gp_struct = OS::g_config->getRootArray("FESL");
	configVar *driver_struct = OS::g_config->getArrayArray(gp_struct, "drivers");
	std::list<configVar *> drivers = OS::g_config->getArrayVariables(driver_struct);
	std::list<configVar *>::iterator it = drivers.begin();
	while (it != drivers.end()) {
		
		configVar *driver_arr = *it;
		const char *bind_ip = OS::g_config->getArrayString(driver_arr, "address");
		int bind_port = OS::g_config->getArrayInt(driver_arr, "port");
		bool ssl = OS::g_config->getArrayInt(driver_arr, "ssl");
		SSLNetIOIFace::ESSL_Type ssl_version = getSSLVersion(driver_arr);

		const char *x509_path = NULL, *rsa_path = NULL;

		const char *stringCrypterPKey = OS::g_config->getArrayString(driver_arr, "stringCrypterPKey");

		if (ssl) {
			x509_path = OS::g_config->getArrayString(driver_arr, "x509");
			rsa_path = OS::g_config->getArrayString(driver_arr, "x509_pkey");
		}

		FESL::PublicInfo server_info;

		server_info.domainPartition = OS::g_config->getArrayString(driver_arr, "domainPartition");
		server_info.subDomain = OS::g_config->getArrayString(driver_arr, "subDomain");

		server_info.messagingHostname = OS::g_config->getArrayString(driver_arr, "messagingHostname");
		server_info.messagingPort = OS::g_config->getArrayInt(driver_arr, "messagingPort");

		server_info.theaterHostname = OS::g_config->getArrayString(driver_arr, "theaterHostname");
		server_info.theaterPort = OS::g_config->getArrayInt(driver_arr, "theaterPort");

		const char *tos_path = OS::g_config->getArrayString(driver_arr, "tosFile");

		server_info.termsOfServiceData = get_file_contents(tos_path);

		FESL::Driver *driver = new FESL::Driver(g_gameserver, bind_ip, bind_port, server_info, stringCrypterPKey, ssl, x509_path, rsa_path, ssl_version);
		OS::LogText(OS::ELogLevel_Info, "Adding FESL Driver: %s:%d (ssl: %d)\n", bind_ip, bind_port, ssl);
		g_gameserver->addNetworkDriver(driver);
		it++;
	}

	g_gameserver->init();
	while(g_running) {
		g_gameserver->tick();
	}

    delete g_gameserver;

    OS::Shutdown();
    return 0;
}

void shutdown() {
    if(g_running) {
        g_running = false;
    }
}
