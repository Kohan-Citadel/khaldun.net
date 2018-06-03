#ifndef _GPDRIVER_H
#define _GPDRIVER_H
#include <stdint.h>
#include "../main.h"
#include <OS/Net/NetDriver.h>

#include "GSPeer.h"

#include <map>
#include <vector>
#ifdef _WIN32
#include <time.h>
#else
#include <sys/time.h>
#endif

#include <OS/GPShared.h>

#define GP_PING_TIME (600)
#define DRIVER_THREAD_TIME 1000

namespace GS {
	class Peer;
	class Driver;
	extern Driver *g_gbl_gp_driver;
	class Driver : public INetDriver {
	public:
		Driver(INetServer *server, const char *host, uint16_t port);
		~Driver();
		void think(bool listener_waiting);

		Peer *FindPeerByProfileID(int profileid);

		const std::vector<INetPeer *> getPeers(bool inc_ref = false);

		INetIOSocket *getListenerSocket() const;
		const std::vector<INetIOSocket *> getSockets() const;
	private:
		void TickConnections();

		std::vector<Peer *> m_connections;

		struct timeval m_server_start;

		static void *TaskThread(OS::CThread *thread);
		std::vector<GS::Peer *> m_peers_to_delete;
		OS::CMutex *mp_mutex;
		OS::CThread *mp_thread;

		INetIOSocket *mp_socket;
	};
}
#endif //_SBDRIVER_H