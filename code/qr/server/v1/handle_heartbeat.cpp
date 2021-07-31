#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <algorithm>
#include <server/QRServer.h>
#include <server/QRDriver.h>
#include <OS/Net/IOIfaces/BSDNetIOInterface.h>
#include <qr/tasks/tasks.h>
#include <tasks/tasks.h>
#include <server/v2.h>
namespace QR {
    void Driver::on_v1_heartbeat_processed(MM::MMTaskResponse response) {
		if(response.error_message != NULL) {
			response.driver->send_v1_error(response.from_address, response.error_message);		
			return;
		} else if(response.challenge.length() != 0) {
            std::stringstream ss;
            ss << "\\secure\\" << response.challenge << "\\final\\";

            std::string message = ss.str();

			OS::Buffer buffer;
            buffer.WriteBuffer(message.c_str(),message.length());
			response.driver->SendPacket(response.from_address, buffer);
		} else {
            response.driver->perform_v1_key_scan(response.from_address);
        }
    }
    void Driver::handle_v1_heartbeat(OS::Address from_address, OS::KVReader reader) {
        MM::ServerInfo server_info;
        server_info.m_address = from_address;
        //std::pair<std::vector<std::pair< std::string, std::string> >::const_iterator, std::vector<std::pair< std::string, std::string> >::const_iterator> GetHead() const;

        std::pair<std::vector<std::pair< std::string, std::string> >::const_iterator, std::vector<std::pair< std::string, std::string> >::const_iterator> it_header = reader.GetHead();
        std::vector<std::pair< std::string, std::string> >::const_iterator it = it_header.first;
        it++; //skip heartbeat setting

        while(it != it_header.second) {
            std::pair< std::string, std::string> p = *it;
            server_info.m_keys[p.first] = p.second;
            it++;
        }
        
        TaskScheduler<MM::MMPushRequest, TaskThreadData> *scheduler = ((QR::Server *)(getServer()))->getScheduler();
        MM::MMPushRequest req;        

        req.from_address = from_address;
        req.v2_instance_key = 0;
        req.driver = this;
        req.server = server_info;
        req.version = 1;
        req.type = MM::EMMPushRequestType_Heartbeat;
        req.callback = on_v1_heartbeat_processed;
        scheduler->AddRequest(req.type, req);
    }


    void Driver::on_v1_heartbeat_data_processed(MM::MMTaskResponse response) {
        if(response.error_message != NULL) {
			response.driver->send_v1_error(response.from_address, response.error_message);		
			return;
		}
    }

    void Driver::handle_v1_heartbeat_data(OS::Address from_address, OS::KVReader reader) {
        MM::ServerInfo server_info;
        server_info.m_address = from_address;

        std::pair<std::vector<std::pair< std::string, std::string> >::const_iterator, std::vector<std::pair< std::string, std::string> >::const_iterator> it_header = reader.GetHead();
        std::vector<std::pair< std::string, std::string> >::const_iterator it = it_header.first;

        while(it != it_header.second) {
            std::pair< std::string, std::string> p = *it;
            int player_id = 0;
            std::string variable_name = p.first;
            if(MM::isPlayerString(p.first, variable_name, player_id)) {
                if(server_info.m_player_keys[variable_name].size() <= player_id) {
                    server_info.m_player_keys[variable_name].push_back(p.second);
                } else {
                    server_info.m_player_keys[variable_name][player_id] = p.second;
                }
            } else {
                server_info.m_keys[variable_name] = p.second;
            }
            
            it++;
        }

        TaskScheduler<MM::MMPushRequest, TaskThreadData> *scheduler = ((QR::Server *)(getServer()))->getScheduler();
        MM::MMPushRequest req;        

        req.from_address = from_address;
        req.v2_instance_key = 0;
        req.driver = this;
        req.server = server_info;
        req.version = 1;
        req.type = MM::EMMPushRequestType_Heartbeat;
        req.callback = on_v1_heartbeat_data_processed;
        scheduler->AddRequest(req.type, req);
    }
}
    