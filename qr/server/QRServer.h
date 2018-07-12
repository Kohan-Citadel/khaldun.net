#ifndef _QRSERVER_H
#define _QRSERVER_H
#include <stdint.h>
#include <OS/Net/NetServer.h>
#include <OS/Task.h>
#include <OS/TaskPool.h>
#define MASTER_PORT 27900
namespace MM {
	class MMPushTask;
	class MMPushRequest;
}
namespace QR {
	class Peer;
	class Server : public INetServer {
	public:
		Server();
		~Server();
		void init();
		void tick();
		void shutdown();
		void SetTaskPool(OS::TaskPool<MM::MMPushTask, MM::MMPushRequest> *pool);
		Peer *find_client(OS::Address address);
	private:
	};
}
#endif //_CHCGAMESERVER_H