#ifndef _NETPEER_H
#define _NETPEER_H
#include <OS/OpenSpy.h>
#include <OS/Buffer.h>
#include <OS/Ref.h>
#include "NetDriver.h"
#include <OS/Net/NetServer.h>
#include <OS/LinkedList.h>

#include <uv.h>
#include <queue>
class INetPeer;
class UVWriteData {
	public:
		UVWriteData(int num_buffers, INetPeer *peer) {
			send_buffers = new OS::Buffer[num_buffers];
			uv_buffers = (uv_buf_t*)malloc(num_buffers * sizeof(uv_buf_t));
			this->peer = peer;
		}
		~UVWriteData() {
			delete[] send_buffers;
			free((void *)uv_buffers);
		}
		OS::Buffer *send_buffers;
		uv_buf_t *uv_buffers;
		INetPeer *peer;
};

class INetPeer : public OS::Ref, public OS::LinkedList<INetPeer *> {
	public:
		INetPeer(INetDriver* driver, uv_tcp_t *sd);
		virtual ~INetPeer();

		void SetAddress(OS::Address address) { m_address = address; }
		virtual void OnConnectionReady() = 0;
		virtual void think(bool packet_waiting) = 0;

		bool ShouldDelete() { return m_delete_flag; };
		bool IsTimeout() { return m_timeout_flag; }
		INetDriver *GetDriver() { return mp_driver; };

		OS::Address getAddress() { return m_address; };

		virtual void Delete(bool timeout = false) = 0;

		static void stream_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);
		virtual void on_stream_read(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) = 0;
		static void write_callback(uv_write_t *req, int status);

		static void close_callback(uv_handle_t *handle);
		void CloseSocket();
	protected:
		static void read_alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
			buf->base = (char*)malloc(suggested_size);
			buf->len = suggested_size;
		}

		uv_tcp_t m_socket;
		INetDriver *mp_driver;
		struct timeval m_last_recv, m_last_ping;
		OS::Address m_address;

		bool m_socket_deleted;
		bool m_delete_flag;
		bool m_timeout_flag;

		static void clear_send_buffer(uv_async_t *handle);
		void append_send_buffer(OS::Buffer buffer, bool close_after = false);
		uv_async_t m_async_send_handle;
		std::queue<OS::Buffer> m_send_buffer;
		bool m_close_when_sendbuffer_empty;
		uv_mutex_t m_send_mutex;
	};
#endif
