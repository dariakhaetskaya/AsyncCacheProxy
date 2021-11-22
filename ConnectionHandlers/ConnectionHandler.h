//
// Created by rey on 11/7/21.
//

#ifndef CACHEPROXY_CONNECTIONHANDLER_H
#define CACHEPROXY_CONNECTIONHANDLER_H

class ConnectionHandler {
public:
    virtual ~ConnectionHandler() = default;

    virtual bool handle(int event) = 0;
};

#endif //CACHEPROXY_CONNECTIONHANDLER_H
