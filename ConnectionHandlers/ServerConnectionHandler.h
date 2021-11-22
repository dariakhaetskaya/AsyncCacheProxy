//
// Created by rey on 11/10/21.
//

#ifndef CACHEPROXY_SERVERCONNECTIONHANDLER_H
#define CACHEPROXY_SERVERCONNECTIONHANDLER_H

#include <iostream>
#include <netdb.h>
#include <codecvt>
#include <locale>
#include <netinet/tcp.h>
#include <bitset>
#include "../CacheProxy.h"

#define BUF_SIZE 1024

class CacheRecord;

class CacheProxy;

class ServerConnectionHandler : public ConnectionHandler {
public:

    ServerConnectionHandler(int socket, CacheRecord *record);

    bool handle(int event) override;

    int getSocket() const { return serverSocket; };

    ~ServerConnectionHandler();

    bool recieve();

private:
    int serverSocket;
    Logger logger;
    CacheRecord *cacheRecord;
};


#endif //CACHEPROXY_SERVERCONNECTIONHANDLER_H
