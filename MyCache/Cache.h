//
// Created by rey on 11/13/21.
//

#ifndef CACHEPROXY_CACHE_H
#define CACHEPROXY_CACHE_H


#include <vector>
#include <map>
#include "../Logger/Logger.h"

class CacheRecord;

class Cache {
private:
    std::map<std::string, CacheRecord *> cache;
    Logger logger;
    bool ran_out_of_memory = false;
    std::vector<int> readyObservers;

public:
    Cache() = default;

    ~Cache();

    bool isCached(const std::string &url);

    CacheRecord *subscribe(const std::string &url, int socket);

    void unsubscribe(const std::string &url, int socket);

    CacheRecord *addRecord(const std::string &url);

    bool isFullyCached(const std::string &url);

    bool ranOutOfMemory() const { return ran_out_of_memory; };

    void setRanOutOfMemory() { ran_out_of_memory = true; };

    std::vector<int> getReadyObservers();

    void clearReadyObservers();
};


class CacheRecord {
private:
    std::vector<int> observers;
    Cache *cache;
    bool is_fully_cached = false;
    std::string data;
    bool is_broken = false;
    Logger logger;
    bool readyForRead = false;

public:
    explicit CacheRecord(Cache *cache);

    ~CacheRecord();

    void setBroken() { is_broken = true; };

    bool isBroken() const { return is_broken; };

    std::string read(size_t start, size_t length) const;

    void write(const std::string &);

    void addObserver(int socket);

    void removeObserver(int socket);

    void setFullyCached();

    bool isFullyCached() const { return is_fully_cached; };

    size_t getLastWrittenChar() { return data.size(); };

    int getObserverCount();

    void setReadyForRead() { readyForRead = true; };

    bool isReadyForRead() const { return readyForRead; };

    std::vector<int> &getObservers() { return observers; };

};


#endif //CACHEPROXY_CACHE_H
