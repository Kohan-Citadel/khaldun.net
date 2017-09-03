#ifndef _QRSERVER_H
#define _QRSERVER_H
#include <stdint.h>
#include <OS/OpenSpy.h>
#include <OS/TaskPool.h>
#include <OS/Net/NetServer.h>
#define NATNEG_PORT 27901
namespace NN {
	class NNQueryTask;
	typedef struct _NNBackendRequest NNBackendRequest;
	class Server : public INetServer {
		public:
			Server();
			void init();
			void tick();
			void shutdown();
			void SetTaskPool(OS::TaskPool<NN::NNQueryTask, NN::NNBackendRequest> *pool);
			void OnGotCookie(int cookie, int client_idx, OS::Address address);
		private:
		
	};
}
#endif //_CHCGAMESERVER_H