#include <algorithm>
#include "TCPDriver.h"

TCPDriver::TCPDriver(INetServer *server, const char *host, uint16_t port, bool proxyHeaders, const char *x509_path, const char *rsa_priv_path, SSLNetIOIFace::ESSL_Type ssl_version) : INetDriver(server) {
    OS::Address bind_address(0, port);
    if(x509_path != NULL && rsa_priv_path != NULL  && ssl_version != SSLNetIOIFace::ESSL_None) {
        mp_net_io_interface = (INetIOInterface<> *)new SSLNetIOIFace::SSLNetIOInterface(ssl_version, rsa_priv_path, x509_path);
    } else {
        mp_net_io_interface = new BSDNetIOInterface<>();
    }
    mp_socket = getNetIOInterface()->BindTCP(bind_address);

    gettimeofday(&m_server_start, NULL);

    m_proxy_headers = proxyHeaders;

    mp_mutex = OS::CreateMutex();
    mp_thread = OS::CreateThread(TCPDriver::TaskThread, this, true);

    mp_head = NULL;
}
TCPDriver::~TCPDriver() {
    mp_thread->SignalExit(true);
    delete mp_mutex;
    delete mp_thread;

    DeleteClients();

    delete mp_socket;
    delete mp_net_io_interface;
}
void TCPDriver::think(bool listener_waiting) {
    if (listener_waiting) {
        std::vector<INetIOSocket *> sockets = getNetIOInterface()->TCPAccept(mp_socket);
        std::vector<INetIOSocket *>::iterator it = sockets.begin();
        while (it != sockets.end()) {
            INetIOSocket *sda = *it;
            if (sda == NULL) return;
            INetPeer *peer = CreatePeer(sda);//new INetPeer(this, sda);

            mp_mutex->lock();

            peer->SetAddress(sda->address);

            m_server->RegisterSocket(peer, m_proxy_headers);
            if(!m_proxy_headers)
                peer->OnConnectionReady();
            
            addPeerToList(peer);
            
            mp_mutex->unlock();
            it++;
        }
    }
}

const std::vector<INetPeer *> TCPDriver::getPeers(bool inc_ref) {
    mp_mutex->lock();
    std::vector<INetPeer *> peers;
    if (mp_head != NULL) {
        INetPeer* p = mp_head;
        do {
            peers.push_back(p);
            if (inc_ref)
                p->IncRef();
        } while ((p = p->GetNext()) != NULL);
    }
    mp_mutex->unlock();
    return peers;
}
INetIOSocket *TCPDriver::getListenerSocket() const {
    return mp_socket;
}
const std::vector<INetIOSocket *> TCPDriver::getSockets() const {
    std::vector<INetIOSocket *> sockets;
    mp_mutex->lock();
    if (mp_head != NULL) {
        INetPeer* p = mp_head;
        do {
            sockets.push_back(p->GetSocket());
        } while ((p = p->GetNext()) != NULL);
    }
    mp_mutex->unlock();
    return sockets;
}

void *TCPDriver::TaskThread(OS::CThread *thread) {
    TCPDriver *driver = (TCPDriver *)thread->getParams();
    while (thread->isRunning()) {
        driver->mp_mutex->lock();
        /*
        std::vector<INetPeer *>::iterator it = driver->m_connections.begin();
        while (it != driver->m_connections.end()) {
            INetPeer *peer = *it;
            if (peer->ShouldDelete() && std::find(driver->m_peers_to_delete.begin(), driver->m_peers_to_delete.end(), peer) == driver->m_peers_to_delete.end()) {
                //marked for delection, dec reference and delete when zero
                it = driver->m_connections.erase(it);
                peer->DecRef();

                driver->m_server->UnregisterSocket(peer);

                driver->m_peers_to_delete.push_back(peer);
                continue;
            }
            it++;
        }*/

        /*it = driver->m_peers_to_delete.begin();
        while (it != driver->m_peers_to_delete.end()) {
            INetPeer *p = *it;
            if (p->GetRefCount() == 0) {
                delete p;
                it = driver->m_peers_to_delete.erase(it);
                continue;
            }
            it++;
        }*/

        driver->TickConnections();
        driver->mp_mutex->unlock();
        OS::Sleep(DRIVER_THREAD_TIME);
    }
    return NULL;
}
void TCPDriver::TickConnections() {
    if (mp_head != NULL) {
        INetPeer* p = mp_head;
        do {
            if (p->ShouldDelete()) {
                INetPeer* next = NULL;
                if (p->GetRefCount() == 1) { //only 1 reference (the drivers reference)
                    if (p == mp_head) {
                        mp_head = NULL;
                    }
                    else if (p->GetNext() != NULL) {
                        next = p->GetNext();
                        p->GetNext()->SetNext(next->GetNext());
                    }
                    p->DecRef();
                    delete p;
                    p = next;
                }
            }
            else {
                p->think(false);
            }
        } while (p && (p = p->GetNext()) != NULL);
    }
}
void TCPDriver::OnPeerMessage(INetPeer *peer) {
    OS::Address source_address, proxy_server_address;
    bool success = getNetIOInterface()->ReadProxyAddress(peer->GetSocket(), source_address, proxy_server_address);

    if(success) {
        m_server->UnregisterSocket(peer);
        m_server->RegisterSocket(peer);
        peer->SetAddress(source_address);
        peer->OnConnectionReady();
    }
}
void TCPDriver::DeleteClients() {
    if (mp_head != NULL) {
        INetPeer* p = mp_head;
        do {
            m_server->UnregisterSocket(p);
            delete p;
        } while ((p = p->GetNext()) != NULL);
    }

}