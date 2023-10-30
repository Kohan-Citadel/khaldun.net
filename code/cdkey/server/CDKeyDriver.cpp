#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include "CDKeyServer.h"

#include <OS/gamespy/gamespy.h>

#include "CDKeyDriver.h"
#include <OS/KVReader.h>

namespace CDKey {
	void alloc_buffer(uv_handle_t* handle,
                            size_t suggested_size,
                            uv_buf_t* buf) {
		buf->base = (char *)malloc(suggested_size);
		buf->len = suggested_size;
	}
	Driver::Driver(INetServer *server, const char *host, uint16_t port) : OS::UDPDriver(server, host, port) {
		gettimeofday(&m_server_start, NULL);
		
		int r = uv_udp_init(uv_default_loop(), &m_recv_udp_socket);
		if(r < 0) {
			OS::LogText(OS::ELogLevel_Error, "[%s:%d] Failed to init UDP socket: %s", host, port, uv_strerror(r));
		}
		uv_handle_set_data((uv_handle_t*) &m_recv_udp_socket, this);
		
    	r = uv_ip4_addr(host, port, &m_recv_addr);
		if(r < 0) {
			OS::LogText(OS::ELogLevel_Error, "[%s:%d] Failed to convert ip4 address: %s", host, port, uv_strerror(r));
		}

    	r = uv_udp_bind(&m_recv_udp_socket, (const struct sockaddr *)&m_recv_addr, UV_UDP_REUSEADDR);
		if(r < 0) {
			OS::LogText(OS::ELogLevel_Error, "[%s:%d] Failed to bind UDP socket: %s", host, port, uv_strerror(r));
		}
    	r = uv_udp_recv_start(&m_recv_udp_socket, alloc_buffer, Driver::on_udp_read);
		if(r < 0) {
			OS::LogText(OS::ELogLevel_Error, "[%s:%d] Failed to start UDP reader: %s", host, port, uv_strerror(r));
		}
	}
	Driver::~Driver() {
	}

	void Driver::on_udp_read(uv_udp_t* handle,
                               ssize_t nread,
                               const uv_buf_t* buf,
                               const struct sockaddr* addr,
                               unsigned flags) {
		Driver *driver = (Driver *)uv_handle_get_data((uv_handle_t*) handle);
		if(nread > 0) {
			OS::Buffer buffer;
			buffer.WriteBuffer(buf->base, nread);
			buffer.resetReadCursor();

			OS::Address address = *(struct sockaddr_in *)addr;


			gamespyxor((char *)buffer.GetHead(), buffer.bytesWritten());
			std::string input = std::string((const char *)buffer.GetHead(), buffer.bytesWritten());
			OS::KVReader data_parser = input;
			std::string command = data_parser.GetKeyByIdx(0);
			OS::LogText(OS::ELogLevel_Info, "[%s] Got key request: %s", address.ToString().c_str(), input.c_str());
			if(command.compare("auth") == 0) {
				driver->handle_auth_packet(address, data_parser);
			}			
		}
		if(buf && buf->base) {
			free((void *)buf->base);
		}
	}
	void Driver::SendPacket(OS::Address to, std::string message) {
		OS::Buffer buffer;
		buffer.WriteBuffer(message.c_str(), message.length());
		gamespyxor((char *)buffer.GetHead(), buffer.bytesWritten());

		SendUDPPacket(to, buffer);
	}
}
