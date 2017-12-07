//
// Created by zhangp on 2017/12/4.
//

#ifndef MUDUO_TODPOLE_GATEWAYSERVER_H
#define MUDUO_TODPOLE_GATEWAYSERVER_H

#include <muduo/net/EventLoop.h>
#include <muduo/base/Logging.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/ThreadLocalSingleton.h>
#include <muduo/net/TcpServer.h>

#include <set>

#include "todpole/ext/net/codecs/LengthHeaderCodec.h"

using namespace muduo;
using namespace muduo::net;

class GatewayServer : noncopyable {

//public:
//    enum HttpRequestParseState {
//        kExpectRequestLine,
//        kExpectHeaders,
//        kExpectBody,
//        kGotAll,
//    };

public:
    GatewayServer(EventLoop *loop,
                  const InetAddress &listenAddr)
            : server_(loop, listenAddr, "GatewayServer"),
              codec_(std::bind(&GatewayServer::onStringMessage, this, _1, _2, _3)) {
        server_.setConnectionCallback(
                std::bind(&GatewayServer::onConnection, this, _1));
        server_.setMessageCallback(
                std::bind(&LengthHeaderCodec::onMessage, &codec_, _1, _2, _3));
    }

    void setThreadNum(int numThreads) {
        server_.setThreadNum(numThreads);
    }

    void start() {
        server_.setThreadInitCallback(std::bind(&GatewayServer::threadInit, this, _1));
        server_.start();
    }

private:
    void onConnection(const TcpConnectionPtr &conn) {
        unsigned int id = this->generateConnectionId();

        LOG_INFO << conn->localAddress().toIpPort() << " -> "
                 << conn->peerAddress().toIpPort() << " -> "
                 << id << " is "
                 << (conn->connected() ? "UP" : "DOWN");

        if (conn->connected()) {
            LocalConnections::instance()[id] = conn;
        } else {
            LocalConnections::instance().erase(id);
        }
    }

    void onStringMessage(const TcpConnectionPtr &,
                         const string &message,
                         Timestamp) {
        this->sendToAll(message);
    }

    void distributeMessage(const string &message) {
        LOG_DEBUG << "begin";
        for (ClientConnectionMap::iterator it = LocalConnections::instance().begin();
             it != LocalConnections::instance().end();
             ++it) {
            codec_.send(get_pointer(it->second), message);
        }
        LOG_DEBUG << "end";
    }

    void threadInit(EventLoop *loop) {
        assert(LocalConnections::pointer() == NULL);
        LocalConnections::instance();
        assert(LocalConnections::pointer() != NULL);
        MutexLockGuard lock(mutex_);
        loops_.insert(loop);
    }

    TcpServer server_;

    MutexLock mutex_;
    LengthHeaderCodec codec_;
    std::set<EventLoop *> loops_;

private:
    /**
     * keep all client connection
     */
    typedef std::map<unsigned int, TcpConnectionPtr> ClientConnectionMap;

    /**
     * use thread local singleton
     */
    typedef ThreadLocalSingleton <ClientConnectionMap> LocalConnections;

    /**
     * index for connection id
     */
    static unsigned int connectionIdIndex_;

    /**
     * generate connection id
     * @return
     */
    unsigned int generateConnectionId();

public:

    /**
     * send to all clients
     * @param message
     */
    void sendToAll(const string &message);

    /**
     * send to clients, use client id
     * @param message
     * @param clientIdList
     * @param excludeClientIdList
     */
    void sendToClients(const string &message, const std::vector<unsigned int> &clientIdList,
                   const std::vector<unsigned int> &excludeClientIdList);

    /**
     * send to one client
     * @param clientId
     * @param message
     */
    void sendToClient(unsigned int clientId, const string &message);
};

#endif //MUDUO_TODPOLE_GATEWAYSERVER_H
