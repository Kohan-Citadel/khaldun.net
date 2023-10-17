#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include "SMServer.h"
#include "SMPeer.h"
#include "SMDriver.h"

namespace SM {
	Driver::Driver(INetServer *server, const char *host, uint16_t port, bool proxyHeaders) : TCPDriver(server, host, port, proxyHeaders) {
	}
	INetPeer *Driver::CreatePeer(uv_tcp_t *sd) {
		return new Peer(this, sd);
	}
}