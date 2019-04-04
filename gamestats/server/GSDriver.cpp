#include "GSDriver.h"
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>

#include "GSServer.h"
#include "GSPeer.h"

#include <OS/GPShared.h>

namespace GS {
	Driver::Driver(INetServer *server, const char *host, uint16_t port) : TCPDriver(server, host, port) {
	}
	Peer *Driver::FindPeerByProfileID(int profileid) {
		std::vector<INetPeer *>::iterator it = m_connections.begin();
		while (it != m_connections.end()) {
			Peer *p = (Peer *)*it;
			if(p->GetProfileID() == profileid) {
				return p;
			}
			it++;
		}
		return NULL;
	}
	INetPeer *Driver::CreatePeer(INetIOSocket *socket) {
		return new Peer(this, socket);
	}
}
