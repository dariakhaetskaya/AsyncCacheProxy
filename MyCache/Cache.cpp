//
// Created by rey on 11/13/21.
//

#include "Cache.h"

bool Cache::isCached(const std::string &url) {
    return cache.find(url) != cache.end();
}

bool Cache::isFullyCached(const std::string &url) {
    return (isCached(url) && cache.find(url)->second->isFullyCached());
}


CacheRecord* Cache::addRecord(const std::string &url) {
    Logger::log("Adding new record for " + url, false);
    auto record = new CacheRecord(this);
    cache.insert(std::make_pair(url, record));
    return record;
}

CacheRecord *Cache::subscribe(const std::string &url, int socket) {
    if (isCached(url)) {
        Logger::log("Subscribing client #" + std::to_string(socket) + " to " + url, true);
        cache.find(url)->second->addObserver(socket);
    } else {
        Logger::log(url + " is not cached.", true);
        return nullptr;
    }
    return cache.find(url)->second;
}

void Cache::unsubscribe(const std::string &url, int socket) {
    if (!isCached(url)) {
        return;
    } else {
        cache.find(url)->second->removeObserver(socket);
    }
}

Cache::~Cache() {
    for (const auto& record : cache){
        delete record.second;
    }
    Logger::log("Cache deleted", true);
}

std::vector<int> Cache::getReadyObservers() {
    for (auto record : cache){
        if (record.second->isReadyForRead()){
            readyObservers.insert(readyObservers.end(), record.second->getObservers().begin(), record.second->getObservers().end());
        }
    }
    return readyObservers;
}

void Cache::clearReadyObservers(){
    readyObservers.clear();
}


