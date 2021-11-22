//
// Created by rey on 11/7/21.
//

#ifndef CACHEPROXY_CACHEPROXY_H
#define CACHEPROXY_CACHEPROXY_H

#include <cstdlib>
#include <poll.h>
#include <fcntl.h>
#include <cstdio>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <set>
#include <map>
#include <vector>
#include "Logger/Logger.h"
#include <netinet/in.h>
#include "ConnectionHandlers/ConnectionHandler.h"
#include "MyCache/Cache.h"

class Cache;

class CacheProxy {

private:
    Logger logger;
    sockaddr_in address{};
    int socketDesc;
    std::vector<pollfd> connections;
    std::map<int, ConnectionHandler *> handlers;
    bool proxyOn = false;
    Cache *cache;

    void disconnectHandler(int _sock);

public:
    Cache *getCache() { return cache; };

    void addNewHandler(int fd, ConnectionHandler *handler);

    CacheProxy(int port);

    ConnectionHandler *getHandlerByFd(int fd);

    ~CacheProxy();

    int createProxySocket(int port);

    bool initProxySocket();

    void addNewConnection(int socket, short event);

    void run();

    void stopProxy();

    void addEvent(int fd, int event);

    void deleteEvent(int fd, int event);

};

#endif //CACHEPROXY_CACHEPROXY_H
