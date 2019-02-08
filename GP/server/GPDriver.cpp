#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <OS/SharedTasks/tasks.h>
#include "GPDriver.h"
#include "GPServer.h"
#include "GPPeer.h"
#include <OS/GPShared.h>

namespace GP {
	Driver::Driver(INetServer *server, const char *host, uint16_t port) : INetDriver(server) {
		OS::Address bind_address(0, port);
		mp_socket = server->getNetIOInterface()->BindTCP(bind_address);

		gettimeofday(&m_server_start, NULL);

		mp_mutex = OS::CreateMutex();
		mp_thread = OS::CreateThread(Driver::TaskThread, this, true);
	}
	Driver::~Driver() {
		mp_thread->SignalExit(true);

		std::vector<Peer *>::iterator it = m_connections.begin();
		while (it != m_connections.end()) {
			Peer *peer = *it;
			delete peer;
			it++;
		}
		delete mp_thread;
		delete mp_mutex;
	}
	void *Driver::TaskThread(OS::CThread *thread) {
		Driver *driver = (Driver *)thread->getParams();
		while (thread->isRunning()) {
			driver->mp_mutex->lock();
			std::vector<Peer *>::iterator it = driver->m_connections.begin();
			while (it != driver->m_connections.end()) {
				Peer *peer = *it;
				if (peer->ShouldDelete() && std::find(driver->m_peers_to_delete.begin(), driver->m_peers_to_delete.end(), peer) == driver->m_peers_to_delete.end()) {
					//marked for delection, dec reference and delete when zero
					it = driver->m_connections.erase(it);
					peer->DecRef();

					driver->m_server->UnregisterSocket(peer);

					driver->m_peers_to_delete.push_back(peer);
					continue;
				}
				it++;
			}

			it = driver->m_peers_to_delete.begin();
			while (it != driver->m_peers_to_delete.end()) {
				GP::Peer *p = *it;
				if (p->GetRefCount() == 0) {
					delete p;
					it = driver->m_peers_to_delete.erase(it);
					continue;
				}
				it++;
			}

			driver->TickConnections();
			driver->mp_mutex->unlock();
			OS::Sleep(DRIVER_THREAD_TIME);
		}
		return NULL;
	}
	void Driver::think(bool listen_waiting) {
		if (listen_waiting) {
			std::vector<INetIOSocket *> sockets = getServer()->getNetIOInterface()->TCPAccept(mp_socket);
			std::vector<INetIOSocket *>::iterator it = sockets.begin();
			while (it != sockets.end()) {
				INetIOSocket *sda = *it;
				if (sda == NULL) return;
				Peer *mp_peer = new Peer(this, sda);

				mp_mutex->lock();
				m_connections.push_back(mp_peer);
				m_server->RegisterSocket(mp_peer);
				mp_mutex->unlock();
				it++;
			}
		}
	}

	void Driver::TickConnections() {
		std::vector<Peer *>::iterator it = m_connections.begin();
		while (it != m_connections.end()) {
			Peer *p = *it;
			p->think(false);
			it++;
		}
	}
	Peer *Driver::FindPeerByProfileID(int profileid) {
		std::vector<Peer *>::iterator it = m_connections.begin();
		while (it != m_connections.end()) {
			Peer *p = *it;
			if(p->GetProfileID() == profileid) {
				return p;
			}
			it++;
		}
		return NULL;
	}
	void Driver::InformStatusUpdate(int from_profileid, GPShared::GPStatus status) {
		std::vector<Peer *>::iterator it = m_connections.begin();
		while (it != m_connections.end()) {
			Peer *p = *it;
			p->inform_status_update(from_profileid, status);
			it++;
		}
	}
	const std::vector<INetPeer *> Driver::getPeers(bool inc_ref) {
		mp_mutex->lock();
		std::vector<INetPeer *> peers;
		std::vector<Peer *>::iterator it = m_connections.begin();
		while (it != m_connections.end()) {
			INetPeer * p = (INetPeer *)*it;
			peers.push_back(p);
			if (inc_ref)
				p->IncRef();
			it++;
		}
		mp_mutex->unlock();
		return peers;
	}

	INetIOSocket *Driver::getListenerSocket() const {
		return mp_socket;
	}
	const std::vector<INetIOSocket *> Driver::getSockets() const {
		std::vector<INetIOSocket *> sockets;
		mp_mutex->lock();
		std::vector<Peer *>::const_iterator it = m_connections.begin();
		while (it != m_connections.end()) {
			Peer *p = *it;
			sockets.push_back(p->GetSocket());
			it++;
		}
		mp_mutex->unlock();
		return sockets;
	}
}
