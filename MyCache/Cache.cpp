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


void Cache::addRecord(const std::string &url, CacheRecord *record) {
    logger.log("Adding new record for " + url, false);
    cache.insert(std::make_pair(url, record));
}

CacheRecord *Cache::subscribe(const std::string &url, int socket) {
    if (isCached(url)) {
        logger.log("Subscribing client #" + std::to_string(socket) + " to " + url, true);
        cache.find(url)->second->addObserver(socket);
    } else {
        logger.log(url + " is not cached.", true);
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
    logger.log("Cache deleted", true);
}
