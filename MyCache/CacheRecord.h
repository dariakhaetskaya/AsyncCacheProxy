//
// Created by rey on 11/13/21.
//

#ifndef CACHEPROXY_CACHERECORD_H
#define CACHEPROXY_CACHERECORD_H


#include <string>
#include "../ConnectionHandlers/ServerConnectionHandler.h"
#include "Cache.h"
#include <vector>


class Cache;

class CacheProxy;

class ServerConnectionHandler;

class CacheRecord {
private:
    std::vector<int> observers;
    Cache *cache;
    CacheProxy *proxy;
    bool is_fully_cached = false;
    std::string data;

public:
    explicit CacheRecord(const std::string &url, CacheProxy *proxy);

    ~CacheRecord();

    std::string read(size_t start, size_t length) const;

    void write(const std::string &);

    void addObserver(int socket);

    void removeObserver(int socket);

    void setFullyCached();

    void notifyCurrentObservers();

    bool isFullyCached() { return is_fully_cached; };

    size_t getLastWrittenChar() { return data.size(); };

    int getObserverCount();




};


#endif //CACHEPROXY_CACHERECORD_H
