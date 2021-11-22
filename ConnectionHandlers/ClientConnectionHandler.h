//
// Created by rey on 11/7/21.
//

#ifndef CACHEPROXY_CLIENTCONNECTIONHANDLER_H
#define CACHEPROXY_CLIENTCONNECTIONHANDLER_H


#include "ConnectionHandler.h"
#include "../CacheProxy.h"
#include "../HTTP_Parser_NodeJS/http_parser.h"
#include "ServerConnectionHandler.h"
#include <netdb.h>
#include <netinet/tcp.h>
#include <bitset>

#define BUF_SIZE 1024

class ServerConnectionHandler;

class ClientConnectionHandler : public ConnectionHandler {
public:
    explicit ClientConnectionHandler(int socket, CacheProxy *proxy);

    int connectToServer(std::string host);

    bool handle(int event) override;

    ~ClientConnectionHandler() = default;

    std::string getURL();

    void setURL(const char *at, size_t len);

    void setHost(const char *at, size_t len);

    int getSocket() const { return clientSocket; };

    std::string getLastField();

    void setLastField(const char *at, size_t len);

    std::string getHeaders();

    void setNewHeader(const std::string& newHeader);

    void resetLastField();

    void createServer(int socket, CacheRecord *record);

    bool writeToServer(const std::string& msg);

    bool tryMakeFirstWriter();

private:
    int clientSocket;
    CacheProxy *proxy;
    std::string lastField;
    http_parser parser;
    http_parser_settings settings;
    std::string url;
    std::string host;
    std::string headers;
    Logger logger;
    ServerConnectionHandler *server;
    CacheRecord *record;
    size_t readPointer = 0;
    bool cachingInParallel = false;
    bool firstWriter = false;

    bool recieve();

    bool becomeFirstWriter();

    bool sendRequest();

    bool initialized = false;

};


#endif //CACHEPROXY_CLIENTCONNECTIONHANDLER_H
