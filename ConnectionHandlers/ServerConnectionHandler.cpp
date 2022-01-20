//
// Created by rey on 11/10/21.
//

#include "ServerConnectionHandler.h"

ServerConnectionHandler::ServerConnectionHandler(int socket, CacheRecord *record) : logger((new Logger())) {
    this->serverSocket = socket;
    this->cacheRecord = record;
}

bool ServerConnectionHandler::handle(int event) {
    if (event & POLLIN) {
        if (!receive()) {
            return false;
        }
    }
    if (event == (POLLIN | POLLHUP)) {
        if (!receive()) {
            return false;
        }
    }
    return true;
}

bool ServerConnectionHandler::receive() {
    char buffer[BUF_SIZE];
    ssize_t len = recv(serverSocket, buffer, BUF_SIZE, 0);
    if (len > 0) {
        cacheRecord->write(std::string(buffer, len));
    }

    if (len == -1) {
        logger->log("Failed to read data from server", true);
        cacheRecord->setBroken();
        return false;
    }

    if (len == 0) {
        logger->log("Server#" + std::to_string(serverSocket) + " done writing. Closing connection", true);
        cacheRecord->setFullyCached();
        return false;
    }

    return true;
}

ServerConnectionHandler::~ServerConnectionHandler() {
    delete logger;
    close(serverSocket);
}

