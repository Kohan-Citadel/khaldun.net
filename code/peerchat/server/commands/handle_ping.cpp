#include <OS/OpenSpy.h>

#include <OS/Buffer.h>
#include <OS/KVReader.h>
#include <sstream>
#include <algorithm>

#include <OS/gamespy/gamespy.h>
#include <tasks/tasks.h>


#include <server/Driver.h>
#include <server/Server.h>
#include <server/Peer.h>

namespace Peerchat {
    void Peer::handle_ping(std::vector<std::string> data_parser) {
		std::string end = "";
		if (data_parser.size() > 1) {
			end = data_parser.at(1);
			if (end[0] == ':') {
				end = end.substr(1);
			}
		}
		uv_clock_gettime(UV_CLOCK_MONOTONIC, &m_last_sent_ping);
        send_message("PONG", end, UserSummary(), ((Peerchat::Server*)GetDriver()->getServer())->getServerName());

		perform_keepalive();
    }
    void Peer::handle_pong(std::vector<std::string> data_parser) {
		perform_keepalive();
    }
}