//
// Created by rey on 11/7/21.
//

#include "ClientConnectionHandler.h"

Logger generalLogger;

static int getUrlFromHTTP(http_parser *parser, const char *at, size_t len) {
    auto *handler = (ClientConnectionHandler *) parser->data;
    Logger::log("Client sent " + std::string(http_method_str(static_cast<http_method>(parser->method)))
                      + " method", true);

    if (parser->method != 1u && parser->method != 2u) {
        Logger::log("Client sent " + std::string(http_method_str(static_cast<http_method>(parser->method)))
                          + " method. Proxy does not currently support it.", false);
        int sock = handler->getSocket();
        char NOT_ALLOWED[59] = "HTTP/1.0 405 METHOD NOT ALLOWED\r\n\r\n Method Not Allowed";
        write(sock, NOT_ALLOWED, 59);
        return 1;
    }

    handler->setURL(at, len);
    Logger::log("Parsed URL : " + handler->getURL(), true);
    return 0;
}

int handleHeaderField(http_parser *parser, const char *at, size_t len) {
    auto *handler = (ClientConnectionHandler *) parser->data;
    handler->setLastField(at, len);
    return 0;
}

int handleHeaderValue(http_parser *parser, const char *at, size_t len) {
    auto *handler = (ClientConnectionHandler *) parser->data;
    if (handler->getLastField() == "Host") {
        handler->setHost(at, len);
    }

    if (handler->getLastField() == "Connection") {
        handler->setNewHeader(handler->getLastField() + ": " + "close" + "\r\n");
    } else {
        handler->setNewHeader(handler->getLastField() + ": " + std::string(at, len) + "\r\n");

    }
    handler->resetLastField();
    return 0;
}

static int handleHeadersComplete(http_parser *parser) {
    auto *handler = (ClientConnectionHandler *) parser->data;
    handler->setNewHeader("\r\n");
    Logger::log("Parsed headers:\n" + handler->getHeaders(), true);
    return 0;
}

bool ClientConnectionHandler::sendRequest() {
    writeToServer("GET " + url + " HTTP/1.0\r\n");
    writeToServer(headers);
    return true;
}

ClientConnectionHandler::ClientConnectionHandler(int sock, CacheProxy *clientProxy) : logger((new Logger())) {
    http_parser_settings_init(&settings);
    http_parser_init(&parser, HTTP_REQUEST);this->clientSocket = sock;
    this->parser.data = this;
    this->proxy = clientProxy;
    this->settings.on_url = getUrlFromHTTP;
    this->settings.on_header_field = handleHeaderField;
    this->settings.on_header_value = handleHeaderValue;
    this->settings.on_headers_complete = handleHeadersComplete;
    this->server = nullptr;
    this->url = "";
    this->host = "";
    this->headers = "";
    this->record = nullptr;
}

bool ClientConnectionHandler::handle(int event) {

    if (event & POLLOUT) {
        if (!initialized || record->isBroken()) {
            return false;
        }

        if (!firstWriter && readPointer >= record->getLastWrittenChar() && !cachingInParallel) {
            Logger::log("Caught up with the first writer. Now caching in parallel", true);
            cachingInParallel = true;
            return true;
        }
        if (readPointer <= record->getLastWrittenChar()) {
            std::string buffer = record->read(readPointer, BUF_SIZE);
            ssize_t ret = send(clientSocket, buffer.data(), buffer.size(), 0);
            if (ret == -1) {
                perror("send failed");
            } else {
                readPointer += buffer.size();
            }
            if ((!record->isFullyCached() && cachingInParallel) || buffer.empty()) {
                proxy->deleteEvent(clientSocket, POLLOUT);
            }
        }

    }

    if (event == (POLLHUP | POLLIN)) {
        return false;
    }

    if (event & POLLIN) {
        if (!receive()) {
            return false;
        }
    }

    return event != (POLLHUP | POLLERR | POLLIN);

}

bool ClientConnectionHandler::receive() {
    char buffer[BUFSIZ];

    std::string request;
    ssize_t len = 1;

    while (len > 0) {
        len = recv(clientSocket, buffer, BUFSIZ, 0);

        if (len < 0) {
            Logger::log("Failed to read data from clientSocket", true);
            return false;
        }

        if (len == 0) {
            Logger::log("Client #" + std::to_string(clientSocket) + " done writing. Closing connection", true);
            proxy->deleteEvent(clientSocket, POLLOUT);
            if (firstWriter && record->getObserverCount() > 1) {
                proxy->makeNewServer(record->getObservers());
            }
            proxy->getCache()->unsubscribe(url, clientSocket);
            return false;
        }

        request.append(buffer, len);
        memset(buffer, 0, BUFSIZ);
        if (len == BUFSIZ) {
            memset(buffer, 0, BUFSIZ);
        } else {
            len = 0;
        }
    }
    Logger::log("buffer = [ " + request + " ]", true);

    if (!initialized) {
        int ret = http_parser_execute(&parser, &settings, request.data(), request.size());

        if (ret != request.size() || parser.http_errno != 0u) {
            Logger::log("Failed to parse http. Errno " + std::to_string(parser.http_errno), false);
            return false;
        }

        if (!proxy->getCache()->isCached(url)) {
            record = proxy->getCache()->addRecord(url);
            // if failed to allocate new record
            if (record == nullptr) {
                Logger::log("Failed to allocate new cache record for " + url, false);
                proxy->stopProxy();
            }
            Logger::log(url + " new record created", false);
            initialized = true;
            return becomeFirstWriter();
        } else if (proxy->getCache()->isFullyCached(url)) {
            record = proxy->getCache()->subscribe(url, clientSocket);
            Logger::log(url + " is cached. Subscribed to cache record", false);
            proxy->addEvent(clientSocket, POLLOUT);
        } else {
            record = proxy->getCache()->subscribe(url, clientSocket);
            Logger::log(url + " is currently caching. Subscribed to cache record", false);
            proxy->addEvent(clientSocket, POLLOUT);
        }

        initialized = true;
    }
    return true;
}

bool ClientConnectionHandler::tryMakeFirstWriter() {
    return becomeFirstWriter();
}

bool ClientConnectionHandler::becomeFirstWriter() {
    firstWriter = true;
    readPointer = 0;
    int serverSocket = connectToServer(host);
    if (serverSocket == -1) {
        Logger::log("Cannot connect to: " + host + " closing connection.", false);
        return false;
    } else {
        createServer(serverSocket, record);
        proxy->getCache()->subscribe(url, clientSocket);
    }
    Logger::log("Connected to server " + host, true);

    if (!sendRequest()) {
        return false;
    }

    Logger::log("Client #" + std::to_string(clientSocket) + " is now first writer", true);
    return true;
}

std::string ClientConnectionHandler::getURL() {
    return url;
}

void ClientConnectionHandler::setURL(const char *at, size_t len) {
    url.append(at, len);
}

std::string ClientConnectionHandler::getLastField() {
    return lastField;
}

void ClientConnectionHandler::setLastField(const char *at, size_t len) {
    lastField.append(at, len);
}

int ClientConnectionHandler::connectToServer(std::string host) {
    int serverSocket;
    if ((serverSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        Logger::log("Failed to open new socket", false);
        return -1;
    }

    struct hostent *hostinfo = gethostbyname(host.data());
    if (hostinfo == NULL) {
        Logger::log("Unknown host" + host, false);
        close(serverSocket);
        return -1;
    }

    struct sockaddr_in sockaddrIn{};
    sockaddrIn.sin_family = AF_INET;
    sockaddrIn.sin_port = htons(80);
    sockaddrIn.sin_addr = *((struct in_addr *) hostinfo->h_addr);

    if ((connect(serverSocket, (struct sockaddr *) &sockaddrIn, sizeof(sockaddrIn))) == -1) {
        Logger::log("Cannot connect to" + host, false);
        close(serverSocket);
        return -1;
    }

    return serverSocket;
}

std::string ClientConnectionHandler::getHeaders() {
    return headers;
}

void ClientConnectionHandler::setNewHeader(const std::string &newHeader) {
    headers.append(newHeader);
}

void ClientConnectionHandler::resetLastField() {
    lastField = "";
}

void ClientConnectionHandler::createServer(int socket, CacheRecord *cacheRecord) {
    server = new ServerConnectionHandler(socket, cacheRecord);
    if (server == nullptr) {
        Logger::log("Failed to create new server", false);
        proxy->stopProxy();
    }
    proxy->addNewConnection(socket, (POLLIN | POLLHUP));
    proxy->addNewHandler(socket, server);
}

bool ClientConnectionHandler::writeToServer(const std::string &msg) {
    ssize_t len = send(server->getSocket(), msg.data(), msg.size(), 0);

    if (len == -1) {
        Logger::log("Failed to send data to server", false);
        return false;
    }

    return true;
}

void ClientConnectionHandler::setHost(const char *at, size_t len) {
    host.append(at, len);
}

ClientConnectionHandler::~ClientConnectionHandler() {
    delete logger;
}


