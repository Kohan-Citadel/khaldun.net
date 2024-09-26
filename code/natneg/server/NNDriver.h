#ifndef _NNDRIVER_H
#define _NNDRIVER_H
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

#define MAX_DATA_SIZE 1400
#define DRIVER_THREAD_TIME 1000
namespace NN {
	class Driver : public OS::UDPDriver {
	public:
		Driver(INetServer *server, const char *host, uint16_t port);
		~Driver();

		const std::vector<INetPeer *> getPeers(bool inc_ref = false);
		void SendPacket(OS::Address to, NatNegPacket *packet);

		void send_init_ack(OS::Address to_address, NatNegPacket *packet, std::string gamename);
		void send_ert_test(OS::Address to_address, NatNegPacket *packet);
		void send_connect(OS::Address to_address, NatNegPacket *packet);
		void send_preinit_ack(OS::Address to_address, NatNegPacket *packet);
		void send_report_ack(OS::Address to_address, NatNegPacket *packet);
	private:
		static void on_udp_read(uv_udp_t* handle,
                               ssize_t nread,
                               const uv_buf_t* buf,
                               const struct sockaddr* addr,
                               unsigned flags);


		static int packetSizeFromType(uint8_t type, uint8_t version);
		void handle_init_packet(OS::Address from, NatNegPacket *packet, std::string gamename);
		void handle_connect_ack_packet(OS::Address from, NatNegPacket *packet);
		void handle_address_check_packet(OS::Address from, NatNegPacket *packet);
		void handle_report_packet(OS::Address from, NatNegPacket *packet);
		void handle_preinit_packet(OS::Address from, NatNegPacket *packet);
		void handle_natify_packet(OS::Address from, NatNegPacket *packet);
		void handle_ert_ack_packet(OS::Address from, NatNegPacket *packet);
		void AddRequest(NNRequestData req);


		uv_timespec64_t m_server_start;
	};
}
#endif //_NNDRIVER_H