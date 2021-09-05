#include <OS/OpenSpy.h>
#include <OS/Buffer.h>
#include <sstream>

#include "../UTPeer.h"
#include "../UTDriver.h"
#include "../UTServer.h"
#include <tasks/tasks.h>

namespace UT {
	void Peer::handle_motd(OS::Buffer recv_buffer) {
		send_motd();
	}
	void Peer::send_motd() {
		if(m_config->is_server) return;
		OS::Buffer send_buffer;

		send_buffer.WriteByte(0x6A);
		send_buffer.WriteByte(0x16);
		
		send_buffer.WriteNTS(m_config->motd);
		if(m_client_version >= 3000)
			send_buffer.WriteInt(0);
		send_packet(send_buffer);
	}

}