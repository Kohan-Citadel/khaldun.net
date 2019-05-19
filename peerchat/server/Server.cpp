#include "Server.h"
#include "Driver.h"
#include <tasks/tasks.h>
namespace Peerchat {
    Server::Server() {
      mp_auth_tasks = TaskShared::InitAuthTasks(this);
      mp_user_tasks = TaskShared::InitUserTasks(this);
      mp_profile_tasks = TaskShared::InitProfileTasks(this);
      mp_peerchat_tasks = Peerchat::InitTasks(this);
    }
    void Server::init() {

    }
    void Server::tick() {
      std::vector<INetDriver *>::iterator it = m_net_drivers.begin();
      while (it != m_net_drivers.end()) {
        INetDriver *driver = *it;
        driver->think(false);
        it++;
      }
      NetworkTick();
    }
    void Server::shutdown() {

    }
    INetPeer *Server::findPeerByProfile(int profile_id, bool inc_ref) {
        return NULL;
    }
    void Server::OnUserMessage(std::string type, std::string from, std::string to, std::string message) {
      std::vector<INetDriver *>::iterator it = m_net_drivers.begin();
      while (it != m_net_drivers.end()) {
        Peerchat::Driver *driver = (Peerchat::Driver *)*it;
        Peerchat::Peer *peer = driver->FindPeerByUserSummary(to);
        if(peer) {
          peer->OnRecvDirectMsg(from, message, type);
        }
        it++;
      }
    }
}