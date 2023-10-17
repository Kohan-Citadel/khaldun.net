#ifndef _TCPDRIVER_H
#define _TCPDRIVER_H

#include <OS/Net/NetPeer.h>
#include <OS/KVReader.h>

namespace OS {
	class TCPDriver : public INetDriver {
		public:
			TCPDriver(INetServer *server, const char *host, uint16_t port);
			virtual ~TCPDriver();
			void think(bool packet_waiting);

			//Linked List iterators
			static bool LLIterator_DeleteAllClients(INetPeer* peer, TCPDriver* driver);
			static bool LLIterator_TickOrDeleteClient(INetPeer* peer, TCPDriver* driver);
			//
		protected:
			virtual INetPeer *CreatePeer(uv_tcp_t *socket) = 0;
			virtual void TickConnections();
			void DeleteClients();
			static void on_new_connection(uv_stream_t *server, int status);

			struct timeval m_server_start;
			uv_tcp_t m_listener_socket;	
	};
}
#endif //_TCPDRIVER_H