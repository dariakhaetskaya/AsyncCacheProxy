//
// Created by rey on 11/13/21.
//

#include "Cache.h"
#include <stdexcept>

void CacheRecord::setFullyCached() {
    this->is_fully_cached = true;
}

int CacheRecord::getObserverCount(){
    return observers.size();
}

CacheRecord::CacheRecord(Cache* cache) {
    this->cache = cache;
}

void CacheRecord::addObserver(int socket) {
    observers.push_back(socket);
    if (is_fully_cached) {
        setReadyForRead();
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
        Logger::log("Proxy ran out of memory. Shutting down...", false);
        cache->setRanOutOfMemory();
        return;
    }
    setReadyForRead();
}

std::string CacheRecord::read(size_t start, size_t length) const {
    if (data.size() - start < length) {
        return data.substr(start, data.size() - start);
    }
    return data.substr(start, length);
}

CacheRecord::~CacheRecord() {
    observers.clear();
    std::string().swap(data);
}

