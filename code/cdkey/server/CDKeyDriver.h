#ifndef _CDKEYDRIVER_H
#define _CDKEYDRIVER_H
#include <stdint.h>
#include <OS/Net/NetDriver.h>
#include <OS/Net/drivers/UDPDriver.h>

#include <queue>
#include <map>
#include <vector>
#ifdef _WIN32
#include <time.h>
#else
#include <sys/time.h>
#endif

#include <OS/KVReader.h>

#define MAX_DATA_SIZE 1400
#define DRIVER_THREAD_TIME 1000
namespace CDKey {
	class Driver : public OS::UDPDriver {
	public:
		Driver(INetServer *server, const char *host, uint16_t port);
		~Driver();


		void handle_auth_packet(OS::Address from, OS::KVReader data_parser);
		void SendPacket(OS::Address to, std::string message);
	private:
		static void on_udp_read(uv_udp_t* handle,
                               ssize_t nread,
                               const uv_buf_t* buf,
                               const struct sockaddr* addr,
                               unsigned flags);

		uv_timespec64_t m_server_start;
	};
}
#endif //_NNDRIVER_H