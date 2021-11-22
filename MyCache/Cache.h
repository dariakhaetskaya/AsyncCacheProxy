//
// Created by rey on 11/13/21.
//

#ifndef CACHEPROXY_CACHE_H
#define CACHEPROXY_CACHE_H


#include "../CacheProxy.h"
#include "CacheRecord.h"

class CacheRecord;

class Cache {
private:
    std::map<std::string, CacheRecord *> cache;

public:
    Cache() = default;

    ~Cache();

    bool isCached(const std::string &url);

    CacheRecord *subscribe(const std::string &url, int socket);

    void unsubscribe(const std::string &url, int socket);

    void addRecord(const std::string &url, CacheRecord *record);

    Logger logger;

    bool isFullyCached(const std::string &url);


};


#endif //CACHEPROXY_CACHE_H
