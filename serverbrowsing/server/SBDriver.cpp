#include "SBDriver.h"
#include <stdio.h>
#include <stdlib.h>
#include  <algorithm>

#include "SBServer.h"

#include "SBPeer.h"
#include "V1Peer.h"
#include "V2Peer.h"

#include <tasks/tasks.h>
namespace SB {
	Driver::Driver(INetServer *server, const char *host, uint16_t port, int version) : INetDriver(server) {
		OS::Address bind_address(0, port);
		mp_socket = server->getNetIOInterface()->BindTCP(bind_address);

		gettimeofday(&m_server_start, NULL);

		m_version = version;

		mp_mutex = OS::CreateMutex();
		mp_thread = OS::CreateThread(Driver::TaskThread, this, true);

	}
	Driver::~Driver() {
		//end all MMQuery tasks first, otherwise can crash here
		mp_thread->SignalExit(true);
		delete mp_thread;
		delete mp_mutex;

		DeleteClients();
	}
	void *Driver::TaskThread(OS::CThread *thread) {
		Driver *driver = (Driver *)thread->getParams();
		while(thread->isRunning()) {
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
				SB::Peer *p = *it;
				if (p->GetRefCount() == 0) {
					delete p;
					it = driver->m_peers_to_delete.erase(it);
					continue;
				}
				it++;
			}

			MM::Server serv;
			while (!driver->m_server_delete_queue.empty()) {
				serv = driver->m_server_delete_queue.front();
				driver->m_server_delete_queue.pop();
				driver->SendDeleteServer(&serv);
			}

			while (!driver->m_server_new_queue.empty()) {
				serv = driver->m_server_new_queue.front();
				driver->m_server_new_queue.pop();
				driver->SendNewServer(&serv);
			}
			while (!driver->m_server_update_queue.empty()) {
				serv = driver->m_server_update_queue.front();
				driver->m_server_update_queue.pop();
				driver->SendUpdateServer(&serv);
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
				Peer *mp_peer = NULL;

				switch (m_version) {
				case 1:
					mp_peer = new V1Peer(this, sda);
					break;
				case 2:
					mp_peer = new V2Peer(this, sda);
					break;
				}

				mp_mutex->lock();
				m_connections.push_back(mp_peer);
				m_server->RegisterSocket(mp_peer);
				mp_mutex->unlock();
				it++;
			}
		}
	}

	INetIOSocket *Driver::getListenerSocket() {
		return mp_socket;
	}

	const std::vector<INetIOSocket *> Driver::getSockets() {
		std::vector<INetIOSocket *> sockets;
		mp_mutex->lock();
		std::vector<Peer *>::iterator it = m_connections.begin();
		while (it != m_connections.end()) {
			Peer *p = *it;
			sockets.push_back(p->GetSocket());
			it++;
		}
		mp_mutex->unlock();
		return sockets;
	}

	void Driver::TickConnections() {
		std::vector<Peer *>::iterator it = m_connections.begin();
		while (it != m_connections.end()) {
			Peer *p = *it;
			p->think(false);
			it++;
		}
	}
	void Driver::SendDeleteServer(MM::Server *server) {
		std::vector<Peer *>::iterator it = m_connections.begin();
		while (it != m_connections.end()) {
			Peer *p = *it;
			p->informDeleteServers(server);
			it++;
		}
	}
	void Driver::SendNewServer(MM::Server *server) {
		std::vector<Peer *>::iterator it = m_connections.begin();
		while (it != m_connections.end()) {
			Peer *p = *it;
			p->informNewServers(server);
			it++;
		}
	}
	void Driver::SendUpdateServer(MM::Server *server) {
		std::vector<Peer *>::iterator it = m_connections.begin();
		while (it != m_connections.end()) {
			Peer *p = *it;
			p->informUpdateServers(server);
			it++;
		}
	}
	void Driver::AddDeleteServer(MM::Server serv) {
		mp_mutex->lock();
		m_server_delete_queue.push(serv);
		mp_mutex->unlock();
	}
	void Driver::AddNewServer(MM::Server serv) {
		mp_mutex->lock();
		m_server_new_queue.push(serv);
		mp_mutex->unlock();
	}
	void Driver::AddUpdateServer(MM::Server serv) {
		mp_mutex->lock();
		m_server_update_queue.push(serv);
		mp_mutex->unlock();
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
		std::vector<INetIOSocket *> ret;
		std::vector<Peer *>::const_iterator it = m_connections.begin();
		while (it != m_connections.end()) {
			Peer *peer = *it;
			ret.push_back(peer->GetSocket());
			it++;
		}
		return ret;
	}
	void Driver::debug_dump() {
		printf("Driver: %p\n", this);
		printf("Peers: \n");
		std::vector<Peer *>::iterator it = m_connections.begin();
		int i = 0;
		while (it != m_connections.end()) {
			INetPeer * peer = (INetPeer *)*it;
			OS::Address address = peer->GetSocket()->address;
			printf("[%s]\n",address.ToString(false).c_str());
			i++;
			it++;
		}
		printf("Peer Count: %d\n", i);
	}
	void Driver::DeleteClients() {
		std::vector<Peer *>::iterator it = m_connections.begin();
		while (it != m_connections.end()) {
			Peer *peer = *it;
			m_server->UnregisterSocket(peer);
			delete peer;
			it++;
		}
	}
}
