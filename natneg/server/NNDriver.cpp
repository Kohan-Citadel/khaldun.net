#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include "NNServer.h"

#include "NNPeer.h"
#include "NNDriver.h"
#include <OS/Net/IOIfaces/BSDNetIOInterface.h>
namespace NN {
	Driver::Driver(INetServer *server, const char *host, uint16_t port) : INetDriver(server) {
		OS::Address bind_address(0, port);
		mp_net_io_interface = new BSDNetIOInterface<>();
		mp_socket = getNetIOInterface()->BindUDP(bind_address);

		gettimeofday(&m_server_start, NULL);

		mp_mutex = OS::CreateMutex();
		mp_thread = OS::CreateThread(Driver::TaskThread, this, true);
	}
	Driver::~Driver() {
		std::vector<Peer *>::iterator it = m_connections.begin();
		while (it != m_connections.end()) {
			Peer *peer = *it;
			m_server->UnregisterSocket(peer);
			delete peer;
			it++;
		}
		delete mp_thread;
		delete mp_mutex;
	}
	void *Driver::TaskThread(OS::CThread *thread) {
		Driver *driver = (Driver *)thread->getParams();
		for (;;) {
			driver->mp_mutex->lock();
			driver->TickConnections();
			std::vector<Peer *>::iterator it = driver->m_connections.begin();
			while (it != driver->m_connections.end()) {
				Peer *peer = *it;
				if (peer->ShouldDelete() && std::find(driver->m_peers_to_delete.begin(), driver->m_peers_to_delete.end(), peer) == driver->m_peers_to_delete.end()) {
					//marked for delection, dec reference and delete when zero
					it = driver->m_connections.erase(it);
					peer->DecRef();
					driver->m_peers_to_delete.push_back(peer);

					driver->m_server->UnregisterSocket(peer);
					continue;
				}
				it++;
			}

			it = driver->m_peers_to_delete.begin();
			while (it != driver->m_peers_to_delete.end()) {
				NN::Peer *p = *it;
				if (p->GetRefCount() == 0) {
					delete p;
					it = driver->m_peers_to_delete.erase(it);
					continue;
				}
				it++;
			}
			driver->mp_mutex->unlock();
			OS::Sleep(DRIVER_THREAD_TIME);
		}
	}
	void Driver::think(bool listener_waiting) {
		mp_mutex->lock();
		TickConnections();
		if (listener_waiting) {
			std::vector<INetIODatagram> datagrams;
			getNetIOInterface()->datagramRecv(mp_socket, datagrams);
			std::vector<INetIODatagram>::iterator it = datagrams.begin();
			while (it != datagrams.end()) {
				INetIODatagram dgram = *it;
				Peer *peer = NULL;
				if (dgram.comm_len == 0) {
					it++;
					continue;
				}
				if (dgram.error_flag) {
					peer = find_client(dgram.address);
					if (peer) {
						peer->Delete(false);
					}
				}
				else {
					NNCookieType cookie;
					get_packet_cookie(dgram, cookie);
					peer = find_or_create(dgram.address, mp_socket, cookie);
					peer->handle_packet(dgram);
				}
				it++;
			}
		}
		mp_mutex->unlock();
	}
	void Driver::get_packet_cookie(INetIODatagram dgram, NNCookieType &cookie) {
		NatNegPacket *packet = (NatNegPacket *)dgram.buffer.GetHead();
		unsigned char NNMagicData[] = { NN_MAGIC_0, NN_MAGIC_1, NN_MAGIC_2, NN_MAGIC_3, NN_MAGIC_4, NN_MAGIC_5 };
		if (memcmp(&NNMagicData, &packet->magic, NATNEG_MAGIC_LEN) == 0) {
			cookie = htonl(packet->cookie);
		}
		else {
			cookie = 0;
		}
	}
	Peer *Driver::find_client(OS::Address address, NNCookieType cookie, bool use_client_info) {
		mp_mutex->lock();
		std::vector<Peer *>::iterator it = m_connections.begin();
		while (it != m_connections.end()) {
			Peer *peer = *it;
			OS::Address peer_address = peer->getAddress();
			if (address == peer_address) {
				if (!use_client_info || peer->GetCookie() == cookie) {
					mp_mutex->unlock();
					return peer;
				}				
			}
			it++;
		}
		mp_mutex->unlock();
		return NULL;
	}
	std::vector<Peer *> Driver::find_clients(NNCookieType cookie, int client_idx, bool inc_ref) {
		std::vector<Peer *> peers;
		mp_mutex->lock();
		std::vector<Peer *>::iterator it = m_connections.begin();
		while (it != m_connections.end()) {
			Peer *peer = *it;
			if (peer->GetCookie() == cookie && peer->GetClientIndex() == client_idx) {
				if (inc_ref) {
					peer->IncRef();
				}
				peers.push_back(peer);
			}
			it++;
		}
		mp_mutex->unlock();
		return peers;
	}
	Peer *Driver::find_client(NNCookieType cookie, int client_idx) {
		mp_mutex->lock();
		std::vector<Peer *>::iterator it = m_connections.begin();
		Peer *return_value = NULL;
		while (it != m_connections.end()) {
			Peer *peer = *it;
			if (peer->GetCookie() == cookie && peer->GetClientIndex() == client_idx) {
				return_value = peer;
				break;
			}
			it++;
		}
		mp_mutex->unlock();
		return return_value;
	}
	Peer *Driver::find_or_create(OS::Address address, INetIOSocket *socket, NNCookieType cookie) {
		mp_mutex->lock();
		std::vector<Peer *>::iterator it = m_connections.begin();
		while (it != m_connections.end()) {
			Peer *peer = *it;
			OS::Address peer_address = peer->getAddress();
			if (address == peer_address && peer->GetCookie() == cookie) {
				mp_mutex->unlock();
				return peer;
			}
			it++;
		}

		
		INetIOSocket *client_socket = new INetIOSocket();
		client_socket->address = address;
		client_socket->sd = socket->sd;
		client_socket->shared_socket = true;

		Peer *ret = new Peer(this, client_socket);
		m_connections.push_back(ret);
		ret->OnConnectionReady();
		mp_mutex->unlock();
		return ret;
	}

	void Driver::TickConnections() {
		std::vector<Peer *>::iterator it = m_connections.begin();
		while (it != m_connections.end()) {
			Peer *p = *it;
			p->think(false);
			it++;
		}
	}

	const std::vector<INetPeer *> Driver::getPeers(bool inc_ref) {
		std::vector<INetPeer *> peers;
		mp_mutex->lock();
		std::vector<Peer *>::iterator it = m_connections.begin();
		while (it != m_connections.end()) {
			INetPeer *p = (INetPeer *)*it;
			if (inc_ref) {
				p->IncRef();
			}
			peers.push_back(p);
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
		ret.push_back(mp_socket);
		return ret;
	}
	INetPeer *Driver::CreatePeer(INetIOSocket *socket) {
		return NULL;
	}
	void Driver::OnPeerMessage(INetPeer *peer) {

	}
}
