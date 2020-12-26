#include <OS/OpenSpy.h>
#if EVTMGR_USE_EPOLL
	#include "EPollNetEventManager.h"
	#include <stdio.h>
	#include <OS/Net/NetServer.h>
	#include <OS/Net/NetPeer.h>
	#include <algorithm>

	EPollNetEventManager::EPollNetEventManager() : INetEventManager(), BSDNetIOInterface() {
		m_epollfd = epoll_create(MAX_EPOLL_EVENTS);

		m_added_drivers = false;
	}
	EPollNetEventManager::~EPollNetEventManager() {
		std::map<void *, EPollDataInfo *>::iterator it =  m_datainfo_map.begin();
		while(it != m_datainfo_map.end()) {
			EPollDataInfo *data = (*it).second;
			free((void *)data);
			it++;
		}
		close(m_epollfd);
	}
	void EPollNetEventManager::run() {
		INetDriver *driver = NULL;
		if(!m_added_drivers) {
			setupDrivers();
		}
		int nr_events = epoll_wait (m_epollfd, (epoll_event *)&m_events, MAX_EPOLL_EVENTS, EPOLL_TIMEOUT);
		for(int i=0;i<nr_events;i++) {
			EPollDataInfo *data = (EPollDataInfo *)m_events[i].data.ptr;
			if(data->is_peer) {
				INetPeer *peer = (INetPeer *)data->ptr;
				std::map<void *, EPollDataInfo *>::iterator it = m_datainfo_map.find(peer);
				if(it != m_datainfo_map.end()) {
					if(data->is_peer_notify_driver) {
						driver = peer->GetDriver();
						driver->OnPeerMessage(peer);
					} else {
						peer->think(true);
					}
					
				}
			} else {
				driver = (INetDriver *)data->ptr;
				driver->think(true);
			}
		}


		//force TCP accept, incase of high connection load, will not block due to non-blocking sockets
		if(nr_events == 0) {
			std::vector<INetDriver *>::iterator it = m_net_drivers.begin();
			while(it != m_net_drivers.end()) {
				INetDriver *driver = *it;
				driver->think(true);
				it++;
			}
		}
		flushSendQueue();
	}
	void EPollNetEventManager::RegisterSocket(INetPeer *peer, bool notify_driver_only) {
		if(peer->GetDriver()->getListenerSocket() != peer->GetSocket()) {
			EPollDataInfo *data_info = (EPollDataInfo *)malloc(sizeof(EPollDataInfo));

			struct epoll_event ev;
			ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
			ev.data.ptr = data_info;

			data_info->ptr = peer;
			data_info->is_peer = true;
			data_info->is_peer_notify_driver = notify_driver_only;

			m_datainfo_map[peer] = data_info;

			epoll_ctl(m_epollfd, EPOLL_CTL_ADD, peer->GetSocket()->sd, &ev);
		}
	}
	void EPollNetEventManager::UnregisterSocket(INetPeer *peer) {
		if(peer->GetDriver()->getListenerSocket() != peer->GetSocket()) {
			std::map<void *, EPollDataInfo *>::iterator it = m_datainfo_map.find(peer);
			if(it != m_datainfo_map.end()) {
				struct epoll_event ev;
				ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
				ev.data.ptr = peer;
				epoll_ctl(m_epollfd, EPOLL_CTL_DEL, peer->GetSocket()->sd, &ev);
				free((void *)m_datainfo_map[peer]);
				m_datainfo_map.erase(it);
			}
		}
	}

	void EPollNetEventManager::setupDrivers() {
		std::vector<INetDriver *>::iterator it = m_net_drivers.begin();
		while(it != m_net_drivers.end()) {
			INetDriver *driver = *it;

			EPollDataInfo *data_info = (EPollDataInfo *)malloc(sizeof(EPollDataInfo));

			data_info->ptr = driver;
			data_info->is_peer = false;

			struct epoll_event ev;
			ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
			ev.data.ptr = data_info;

			m_datainfo_map[driver] = data_info;

			epoll_ctl(m_epollfd, EPOLL_CTL_ADD, driver->getListenerSocket()->sd, &ev);
			it++;
		}
		m_added_drivers = true;
	}
#endif
