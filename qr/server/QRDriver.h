#ifndef _QRDRIVER_H
#define _QRDRIVER_H
#include <stdint.h>
#include <queue>
#include <OS/OpenSpy.h>
#include <OS/Mutex.h>
#include <OS/Net/NetDriver.h>

#include "QRPeer.h"


#include <map>
#include <vector>
#ifdef _WIN32
#include <time.h>
#else
#include <sys/time.h>
#endif

#define MAX_DATA_SIZE 1400
#define DRIVER_THREAD_TIME 1000
namespace QR {
	class Peer;
	

	class Driver : public INetDriver {
	public:
		Driver(INetServer *server, const char *host, uint16_t port);
		~Driver();
		void think(bool listener_waiting);

		Peer *find_client(OS::Address address);
		Peer *find_or_create(OS::Address address, INetIOSocket *socket, int version = 2);

		const std::vector<INetPeer *> getPeers(bool inc_ref = false);

		INetIOSocket *getListenerSocket() const;
		const std::vector<INetIOSocket *> getSockets() const;
		void OnPeerMessage(INetPeer *peer);
	private:
		static void *TaskThread(OS::CThread *thread);
		void TickConnections();

		std::vector<Peer *> m_connections;
		std::vector<Peer *> m_peers_to_delete;
		
		struct timeval m_server_start;

		INetIOSocket *mp_socket;

		OS::CMutex *mp_mutex;
		OS::CThread *mp_thread;

	};
}
#endif //_QRDRIVER_H