//
// Created by rey on 11/13/21.
//

#include "CacheRecord.h"
#include "../ConnectionHandlers/ClientConnectionHandler.h"
#include <stdexcept>

void CacheRecord::setFullyCached() {
    this->is_fully_cached = true;
}

int CacheRecord::getObserverCount(){
    return observers.size();
}

CacheRecord::CacheRecord(const std::string& url, CacheProxy *clientProxy) {
    this->cache = clientProxy->getCache();
    this->cache->addRecord(url, this);
    this->proxy = clientProxy;
}

void CacheRecord::addObserver(int socket) {
    observers.push_back(socket);
    if (is_fully_cached) {
        proxy->addEvent(socket, POLLOUT);
    }
}

void CacheRecord::removeObserver(int socket) {
    for (auto iter = observers.begin(); iter != observers.end(); ++iter) {
        if ((*iter) == socket) {
            observers.erase(iter);
            break;
        }
    }
}

void CacheRecord::write(const std::string &str) {
    try {
        data.append(str);
    }
    catch (std::bad_alloc &a) {
        cache->logger.log("Proxy ran out of memory. Shutting down...", false);
        proxy->stopProxy();
        return;
    }
    for (auto observer : observers) {
        proxy->addEvent(observer, POLLOUT);
    }
}

std::string CacheRecord::read(size_t start, size_t length) const {
    if (data.size() - start < length) {
        return data.substr(start, data.size() - start);
    }
    return data.substr(start, length);
}

void CacheRecord::notifyCurrentObservers() {
    // notify all observers that first writer (that was connected to server) died
    for (auto observer : observers) {
        proxy->addEvent(observer, POLLOUT);
    }

    for (auto observer : observers) {
        auto* client = dynamic_cast<ClientConnectionHandler *>(proxy->getHandlerByFd(observer));
        if (client != nullptr && client->tryMakeFirstWriter()){
            return;
        }
    }
}

CacheRecord::~CacheRecord() {
    observers.clear();
    data.clear();
}
