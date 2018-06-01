#include "SBPeer.h"
#include "SBDriver.h"
#include <OS/OpenSpy.h>

namespace SB {
	Peer::Peer(Driver *driver, INetIOSocket *sd, int version) : INetPeer(driver, sd) {
		mp_driver = driver;
		m_delete_flag = false;
		m_timeout_flag = false;
		gettimeofday(&m_last_ping, NULL);
		gettimeofday(&m_last_recv, NULL);

		m_version = version;
		
		mp_mutex = OS::CreateMutex();

		m_peer_stats.version = version;
		m_peer_stats.m_address = m_sd->address;
		m_peer_stats.bytes_in = 0;
		m_peer_stats.bytes_out = 0;
		m_peer_stats.packets_in = 0;
		m_peer_stats.packets_out = 0;
		m_peer_stats.total_requests = 0;
		m_peer_stats.from_game.gamename[0] = 0;
		m_peer_stats.from_game.gameid = 0;
		m_peer_stats.disconnected = false;

		OS::LogText(OS::ELogLevel_Info, "[%s] New connection version %d",m_sd->address.ToString().c_str(), m_version);
	}
	Peer::~Peer() {
		OS::LogText(OS::ELogLevel_Info, "[%s] Connection closed, timeout: %d", m_sd->address.ToString().c_str(), m_timeout_flag);
		delete mp_mutex;
	}

	bool Peer::serverMatchesLastReq(MM::Server *server, bool require_push_flag) {
		/*
		if(require_push_flag && !m_last_list_req.push_updates) {
			return false;
		}*/
		if(server->game.gameid == m_last_list_req.m_for_game.gameid) {
			return true;
		}
		return false;
	}
	sServerCache Peer::FindServerByIP(OS::Address address) {
		sServerCache ret;
		ret.full_keys = false;
		ret.key[0] = 0;
		mp_mutex->lock();
		std::vector<sServerCache>::iterator it = m_visible_servers.begin();
		while(it != m_visible_servers.end()) {
			sServerCache cache = *it;
			if(cache.wan_address.ip == address.ip && cache.wan_address.port == address.port) {
				mp_mutex->unlock();
				return cache;
			}
			it++;
		}
		mp_mutex->unlock();
		return ret;		
	}
	void Peer::DeleteServerFromCacheByIP(OS::Address address) {
		std::vector<sServerCache>::iterator it = m_visible_servers.begin();
		mp_mutex->lock();
		while(it != m_visible_servers.end()) {
			sServerCache cache = *it;
			if(cache.wan_address.ip == address.ip && cache.wan_address.port == address.port) {
				it = m_visible_servers.erase(it);
				continue;
			}
			it++;
		}
		mp_mutex->unlock();
	}
	void Peer::DeleteServerFromCacheByKey(std::string key) {
		mp_mutex->lock();
		std::vector<sServerCache>::iterator it = m_visible_servers.begin();
		while(it != m_visible_servers.end()) {
			sServerCache cache = *it;
			if(cache.key.compare(key) == 0) {
				it = m_visible_servers.erase(it);
				continue;
			}
			it++;
		}
		mp_mutex->unlock();
	}
	void Peer::cacheServer(MM::Server *server, bool full_keys) {
		sServerCache item;
		mp_mutex->lock();
		if(FindServerByIP(server->wan_address).key[0] == 0) {
			item.wan_address = server->wan_address;
			item.key = server->key;
			item.full_keys = full_keys;
			m_visible_servers.push_back(item);
		}
		mp_mutex->unlock();
	}
	sServerCache Peer::FindServerByKey(std::string key) {
		mp_mutex->lock();
		sServerCache ret;
		ret.full_keys = false;
		ret.key[0] = 0;
		std::vector<sServerCache>::iterator it = m_visible_servers.begin();
		while(it != m_visible_servers.end()) {
			sServerCache cache = *it;
			if(cache.key.compare(key) == 0) {
				mp_mutex->unlock();
				return cache;
			}
			it++;
		}
		mp_mutex->unlock();
		return ret;		
	}
	void Peer::AddRequest(MM::MMQueryRequest req) {
		if (req.type != MM::EMMQueryRequestType_GetGameInfoByGameName && req.type != MM::EMMQueryRequestType_GetGameInfoPairByGameName) {
			if (m_game.secretkey[0] == 0) {
				m_pending_request_list.push(req);
				return;
			}
		}
		req.peer = this;
		req.peer->IncRef();
		req.driver = mp_driver;

		m_peer_stats.total_requests++;
		MM::m_task_pool->AddRequest(req);
	}
	void Peer::FlushPendingRequests() {
		while (!m_pending_request_list.empty()) {
			MM::MMQueryRequest req = m_pending_request_list.front();
			req.peer = this;
			req.peer->IncRef();
			req.driver = mp_driver;
			m_peer_stats.total_requests++;
			MM::m_task_pool->AddRequest(req);
			m_pending_request_list.pop();
		}
	}

	OS::MetricValue Peer::GetMetricItemFromStats(PeerStats stats) {
		OS::MetricValue arr_value, value;
		value.value._str = stats.m_address.ToString(false);
		value.key = "ip";
		value.type = OS::MetricType_String;
		arr_value.arr_value.values.push_back(std::pair<OS::MetricType, struct OS::_Value>(OS::MetricType_String, value));

		value.value._int = stats.disconnected;
		value.key = "disconnected";
		value.type = OS::MetricType_Integer;
		arr_value.arr_value.values.push_back(std::pair<OS::MetricType, struct OS::_Value>(OS::MetricType_String, value));		

		value.type = OS::MetricType_Integer;
		value.value._int = stats.bytes_in;
		value.key = "bytes_in";
		arr_value.arr_value.values.push_back(std::pair<OS::MetricType, struct OS::_Value>(OS::MetricType_String, value));

		value.value._int = stats.bytes_out;
		value.key = "bytes_out";
		arr_value.arr_value.values.push_back(std::pair<OS::MetricType, struct OS::_Value>(OS::MetricType_String, value));

		value.value._int = stats.packets_in;
		value.key = "packets_in";
		arr_value.arr_value.values.push_back(std::pair<OS::MetricType, struct OS::_Value>(OS::MetricType_String, value));

		value.value._int = stats.packets_out;
		value.key = "packets_out";
		arr_value.arr_value.values.push_back(std::pair<OS::MetricType, struct OS::_Value>(OS::MetricType_String, value));

		value.value._int = stats.total_requests;
		value.key = "pending_requests";
		arr_value.arr_value.values.push_back(std::pair<OS::MetricType, struct OS::_Value>(OS::MetricType_String, value));
		arr_value.type = OS::MetricType_Array;
			
		value.type = OS::MetricType_String;	
		value.key = "gamename";
		value.value._str = stats.from_game.gamename;
		arr_value.arr_value.values.push_back(std::pair<OS::MetricType, struct OS::_Value>(OS::MetricType_String, value));

		value.type = OS::MetricType_Integer;	
		value.key = "version";
		value.value._int = stats.version;
		arr_value.arr_value.values.push_back(std::pair<OS::MetricType, struct OS::_Value>(OS::MetricType_String, value));

		arr_value.key = stats.m_address.ToString(false);
		return arr_value;
	}
	void Peer::ResetMetrics() {
		m_peer_stats.bytes_in = 0;
		m_peer_stats.bytes_out = 0;
		m_peer_stats.packets_in = 0;
		m_peer_stats.packets_out = 0;
		m_peer_stats.total_requests = 0;
	}
	OS::MetricInstance Peer::GetMetrics() {
		OS::MetricInstance peer_metric;

		peer_metric.value = GetMetricItemFromStats(m_peer_stats);
		peer_metric.key = "peer";

		ResetMetrics();

		return peer_metric;
	}
	void Peer::Delete(bool timeout) {
		m_timeout_flag = timeout;
		m_delete_flag = true;
	}
}