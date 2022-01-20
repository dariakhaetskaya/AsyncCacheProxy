//
// Created by rey on 11/7/21.
//
#include "CacheProxy.h"

bool interrupted = false;

void interruptProxy(int signal){
    interrupted = true;
}

int CacheProxy::createProxySocket(int port) {

    this->address.sin_port = htons(port);
    this->address.sin_family = AF_INET;
    this->address.sin_addr.s_addr = htonl(INADDR_ANY);
    return socket(AF_INET, SOCK_STREAM, 0);
}

void CacheProxy::stopProxy(){
    this->proxyOn = false;
}


bool CacheProxy::initProxySocket() {
    if (bind(socketDesc, (sockaddr *) &address, sizeof(address)) == -1) {
        Logger::log("CacheProxy :: Socket bind failed. Shutting down.", false);
        return false;
    }

    if (fcntl(socketDesc, F_SETFL, O_NONBLOCK) == -1) {
        Logger::log("CacheProxy :: Failed to set non-blocking flag to socketDesc. Shutting down.", false);
        return false;
    }

    if (listen(socketDesc, 510) == -1) {
        Logger::log("CacheProxy :: listen(2) failed. Shutting down.", false);
        return false;
    }

    return true;
}

void CacheProxy::addNewConnection(int socket, short event) {
    pollfd fd{};
    fd.fd = socket;
    fd.events = event;
    fd.revents = 0;
    connections.push_back(fd);
}

CacheProxy::CacheProxy(int port) {
    Logger::log("CacheProxy :: Starting proxy on port " + std::to_string(port), false);

    this->socketDesc = createProxySocket(port);
    if (socketDesc == -1) {
        Logger::log("CacheProxy :: Socket creation failed. Shutting down.", false);
        return;
    }

    if (!initProxySocket()) {
        Logger::log("CacheProxy :: Shutting down due to an internal error.", false);
        return;
    }
    addNewConnection(socketDesc, POLLIN | POLLHUP);
    this->proxyOn = true;
    this->cache = new Cache();
    if (cache == nullptr){
        Logger::log("Failed to create cache", false);
        stopProxy();
    }
    memset(&address, 0, sizeof(address));
    sigset(SIGINT, interruptProxy);
}

ConnectionHandler* CacheProxy::getHandlerByFd(int fd){
    ConnectionHandler* handler;
    try {
        handler = handlers.at(fd);
    } catch (std::out_of_range &e) {
        return nullptr;
    }
    return handler;
}

void CacheProxy::disconnectHandler(int socket) {
    Logger::log("Closing #" + std::to_string(socket), true);

    for (auto iter = connections.begin(); iter != connections.end(); ++iter) {
        if ((*iter).fd == socket) {
            connections.erase(iter);
            break;
        }
    }

    auto handler = handlers.at(socket);
    handlers.erase(socket);
    close(socket);
    delete handler;
}

void CacheProxy::addEvent(int fd, int event) {
    for (auto &connection : connections) {
        if (connection.fd == fd) {
            connection.events |= event;
            break;
        }
    }

}

void CacheProxy::deleteEvent(int fd, int event) {

    for (auto &connection : connections) {
        if (connection.fd == fd) {
            connection.events &= ~event;
            break;
        }
    }

}

void CacheProxy::run() {
    int selectedDescNum;
    int timeOut = 100000;

    while (proxyOn && !interrupted && !cache->ranOutOfMemory()) {
        setPollOutEventToObservers(cache->getReadyObservers());
        if ((selectedDescNum = poll(connections.data(), connections.size(), timeOut)) == -1) {
            Logger::log("CacheProxy :: run poll(2) internal error. Shutting down", false);
            return;

        }

        if (selectedDescNum > 0) {
            if (connections[0].revents == POLLIN) {
                connections[0].revents = 0;

                int newClientFD;

                if ((newClientFD = accept(socketDesc, nullptr, nullptr)) == -1) {
                    Logger::log("CacheProxy :: Failed to accept new connection", false);
                    continue;
                }

                addNewConnection(newClientFD, POLLIN | POLLHUP);
                ConnectionHandler *client = new ClientConnectionHandler(newClientFD, this);
                if (client == nullptr){
                    Logger::log("Failed to create new client handler", false);
                    stopProxy();
                }
                addNewHandler(newClientFD, client);
                Logger::log("CacheProxy :: Accepted new connection from client #" + std::to_string(newClientFD), true);
            }

            for (int i = 1; i < connections.size(); ++i) {
                short eventCount = connections[i].revents;

                if (eventCount > 0) {

                    if (!(handlers.at(connections[i].fd)->handle(connections[i].revents))) {
                        disconnectHandler(connections[i].fd);
                        Logger::log("CacheProxy :: Closed connection with #" + std::to_string(connections[i].fd), true);
                        connections[i].revents = 0;
                        continue;
                    }

                    connections[i].revents = 0;

                }
            }
        }
    }
}

void CacheProxy::addNewHandler(int fd, ConnectionHandler *handler) {
    handlers.insert(std::make_pair(fd, handler));
}

CacheProxy::~CacheProxy() {
    for (auto handler : handlers) {
        disconnectHandler(handler.first);
    }
    close(socketDesc);
    delete cache;
}

void CacheProxy::makeNewServer(const std::vector<int>& observers) {
    for (auto observer : observers) {
        addEvent(observer, POLLOUT);
        auto* client = dynamic_cast<ClientConnectionHandler *>(getHandlerByFd(observer));
        if (client != nullptr && client->tryMakeFirstWriter()){
            return;
        }
    }
}

void CacheProxy::setPollOutEventToObservers(const std::vector<int>& observers){
    for (auto observer : observers){
        addEvent(observer, POLLOUT);
    }
    cache->clearReadyObservers();
}
