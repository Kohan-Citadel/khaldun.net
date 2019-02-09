#include <OS/OpenSpy.h>

#include <OS/SharedTasks/tasks.h>
#include <server/FESLServer.h>
#include <server/FESLDriver.h>
#include <server/FESLPeer.h>


#include <sstream>
namespace FESL {
	bool Peer::m_acct_gettos_handler(OS::KVReader kv_list) {
		std::ostringstream s;
		s << "TXN=GetTos\n";
		s << "tos=\"" << ((FESL::Driver *)GetDriver())->GetServerInfo().termsOfServiceData << "\"\n";

		SendPacket(FESL_TYPE_FSYS, s.str());
		return true;
	}
}