#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <peerchat/tasks/tasks.h>
#include <OS/SharedTasks/tasks.h>
#include "Driver.h"

#include <sstream>
namespace Peerchat {
    Driver::Driver(INetServer *server, const char *host, uint16_t port, bool proxyHeaders) : TCPDriver(server, host, port, proxyHeaders) {

    }
    Peer *Driver::FindPeerByProfileID(int profileid) {
        return NULL;
    }
    Peer *Driver::FindPeerByUserSummary(std::string summary_string) {
		mp_mutex->lock();
		std::vector<INetPeer *>::iterator it = m_connections.begin();
		while (it != m_connections.end()) {
			Peer *peer = (Peer *)*it;
			if (stricmp(peer->GetUserDetails().ToString().c_str(), summary_string.c_str()) == 0 && !peer->ShouldDelete()) {
				mp_mutex->unlock();
				return peer;
			}
			it++;
		}
		mp_mutex->unlock();
		return NULL;   
    }
    INetPeer *Driver::CreatePeer(INetIOSocket *socket) {
        return new Peer(this, socket);
    }
	void Driver::SendUserMessageToVisibleUsers(std::string fromSummary, std::string messageType, std::string message, bool includeSelf) {
		mp_mutex->lock();
		std::vector<INetPeer *>::iterator it = m_connections.begin();
		while (it != m_connections.end()) {
			Peer *peer = (Peer *)*it;
			bool summaryMatch = peer->GetUserDetails().ToString().compare(fromSummary);
			if(summaryMatch && includeSelf) {
				peer->send_message(messageType, message, fromSummary);
			} else if(!summaryMatch) {
				peer->send_message(messageType, message, fromSummary);
			}
			it++;
		}
		mp_mutex->unlock();
	}
	void Driver::OnChannelMessage(std::string type, std::string from, ChannelSummary channel, std::string message, std::string target, bool includeSelf) {
		mp_mutex->lock();
		std::vector<INetPeer *>::iterator it = m_connections.begin();
		while (it != m_connections.end()) {
			Peer *peer = (Peer *)*it;
			if(peer->GetChannelFlags(channel.channel_id) & EUserChannelFlag_IsInChannel || (includeSelf && peer->GetUserDetails().ToString().compare(from) == 0)) {
				peer->send_message(type, message,from, channel.channel_name, target);
			}
			it++;
		}
		mp_mutex->unlock();
	}

	void Driver::OnSetUserChannelKeys(ChannelSummary summary, UserSummary user_summary, OS::KVReader keys) {
		mp_mutex->lock();
		std::vector<INetPeer *>::iterator it = m_connections.begin();
		std::ostringstream ss;
		ss << summary.channel_name << " " << user_summary.nick << " BCAST :" << keys.ToString();
		while (it != m_connections.end()) {
			Peer *peer = (Peer *)*it;
			if(peer->GetChannelFlags(summary.channel_id) & EUserChannelFlag_IsInChannel) {
				peer->send_numeric(702,ss.str(), true, summary.channel_name, false);
			}
			it++;
		}
		mp_mutex->unlock();
	}
	void Driver::OnSetChannelKeys(ChannelSummary summary, OS::KVReader keys) {
		mp_mutex->lock();
		std::vector<INetPeer *>::iterator it = m_connections.begin();
		std::ostringstream ss;
		ss << summary.channel_name << " BCAST :" << keys.ToString();
		while (it != m_connections.end()) {
			Peer *peer = (Peer *)*it;
			if(peer->GetChannelFlags(summary.channel_id) & EUserChannelFlag_IsInChannel) {
				peer->send_numeric(704,ss.str(), true, summary.channel_name, false);
			}
			it++;
		}
		mp_mutex->unlock();
	}
	void Driver::OnChannelBroadcast(std::string type, std::string target, std::vector<int> channel_list, std::string message, bool includeSelf) {
		mp_mutex->lock();
		std::vector<INetPeer *>::iterator it = m_connections.begin();
		while (it != m_connections.end()) {
			Peer *peer = (Peer *)*(it++);
			if (includeSelf && peer->GetUserDetails().ToString().compare(target) == 0) {
				peer->send_message(type, message, target);
				continue;
			}
			std::vector<int>::iterator it2 = channel_list.begin();
			while(it2 != channel_list.end()) {
				if(peer->GetChannelFlags(*it2) & EUserChannelFlag_IsInChannel) {
					peer->send_message(type, message,target);
					break;
				}
				it2++;
			}
		}
		mp_mutex->unlock();
	}
}