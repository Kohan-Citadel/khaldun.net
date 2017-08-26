#ifndef _QRPEER_H
#define _QRPEER_H
#include "../main.h"

#include "QRDriver.h"

#include <OS/legacy/gsmsalg.h>

#include <OS/Net/NetPeer.h>
#include <OS/Ref.h>
#include "MMPush.h"

#define REQUEST_KEY_LEN 4
#define CHALLENGE_LEN 20
#define KV_MAX_LEN 64
namespace QR {
	class Driver;

	class Peer : public INetPeer {
	public:
		Peer(Driver *driver, struct sockaddr_in *address_info, int sd, int version);
		virtual ~Peer();
		
		virtual void think(bool listener_waiting) = 0;
		virtual void handle_packet(char *recvbuf, int len) = 0;		

		const struct sockaddr_in *getAddress() { return &m_address_info; }
		bool ShouldDelete() { return m_delete_flag; };
		void SetDelete(bool del) { m_delete_flag = del; };
		bool IsTimeout() { return m_timeout_flag; }

		virtual void send_error(bool die, const char *fmt, ...) = 0;
		virtual void SendClientMessage(uint8_t *data, int data_len) = 0;
		


		virtual void OnGetGameInfo(OS::GameData game_info, void *extra) = 0;
		virtual void OnRegisteredServer(int pk_id, void *extra) = 0;
	protected:



		bool isTeamString(const char *string);

		Driver *mp_driver;

		struct sockaddr_in m_address_info;

		struct timeval m_last_recv, m_last_ping;

		bool m_delete_flag;
		bool m_timeout_flag;

		int m_version;

		bool m_server_pushed;
		bool m_sent_game_query;

		MM::ServerInfo m_server_info;
	};
}
#endif //_QRPEER_H