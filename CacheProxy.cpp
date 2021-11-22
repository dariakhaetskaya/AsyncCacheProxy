//
// Created by rey on 11/7/21.
//

#include <iostream>
#include <bitset>
#include "CacheProxy.h"
#include "ConnectionHandlers/ClientConnectionHandler.h"

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
        logger.log("CacheProxy :: Socket bind failed. Shutting down.", false);
        return false;
    }

    if (fcntl(socketDesc, F_SETFL, O_NONBLOCK) == -1) {
        logger.log("CacheProxy :: Failed to set non-blocking flag to socketDesc. Shutting down.", false);
        return false;
    }

    if (listen(socketDesc, 510) == -1) {
        logger.log("CacheProxy :: listen(2) failed. Shutting down.", false);
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
    logger.log("CacheProxy :: Starting proxy on port " + std::to_string(port), false);

    this->socketDesc = createProxySocket(port);
    if (socketDesc == -1) {
        logger.log("CacheProxy :: Socket creation failed. Shutting down.", false);
        return;
    }

    if (!initProxySocket()) {
        logger.log("CacheProxy :: Shutting down due to an internal error.", false);
        return;
    }
    addNewConnection(socketDesc, POLLIN | POLLHUP);
    this->proxyOn = true;
    this->cache = new Cache();
    if (cache == nullptr){
        logger.log("Failed to create cache", false);
        stopProxy();
    }
    memset(&address, 0, sizeof(address));
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
    auto handler = handlers.at(socket);
    handlers.erase(socket);
    for (auto iter = connections.begin(); iter != connections.end(); ++iter)
        if ((*iter).fd == socket) {
            connections.erase(iter);
            break;
        }
    logger.log("Closing #" + std::to_string(socket), true);
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

    while (proxyOn) {
        if ((selectedDescNum = poll(connections.data(), connections.size(), timeOut)) == -1) {
            logger.log("CacheProxy :: run poll(2) internal error. Shutting down", false);
            return;

        }

        if (selectedDescNum > 0) {
            if (connections[0].revents == POLLIN) {
                connections[0].revents = 0;

                int newClientFD;

                if ((newClientFD = accept(socketDesc, nullptr, nullptr)) == -1) {
                    logger.log("CacheProxy :: Failed to accept new connection", false);
                    continue;
                }

                addNewConnection(newClientFD, POLLIN | POLLHUP);
                ConnectionHandler *client = new ClientConnectionHandler(newClientFD, this);
                if (client == nullptr){
                    logger.log("Failed to create new client handler", false);
                    stopProxy();
                }
                addNewHandler(newClientFD, client);
                logger.log("CacheProxy :: Accepted new connection from client #" + std::to_string(newClientFD), true);
            }

            for (int i = 1; i < connections.size(); ++i) {
                short eventCount = connections[i].revents;
//                std::cout << "CacheProxy :: starting to handle #" + std::to_string(connections[i].fd) +
//                             " with events = " << std::bitset<8>(connections[i].events) << std::endl;

                if (eventCount > 0) {

//                    std::cout << "CacheProxy :: starting to handle #" + std::to_string(connections[i].fd) +
//                        " with revents = " << std::bitset<8>(connections[i].revents) << std::endl;

                    if (!(handlers.at(connections[i].fd)->handle(connections[i].revents))) {
                        disconnectHandler(connections[i].fd);
                        logger.log("CacheProxy :: Closed connection with #" + std::to_string(connections[i].fd), true);
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
    for (auto handler : handlers){
        disconnectHandler(handler.first);
    }
    close(socketDesc);
    delete cache;
}
