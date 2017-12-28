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

#include "todpole/ext/net/codecs/WebSocketCodec.h"

using namespace muduo;
using namespace muduo::net;

namespace muduo::ext {

    class GatewayServer : noncopyable {

    public:

        typedef std::function<void (
        const TcpConnectionPtr&
        )>
        OnConnectionCallback;

        typedef std::function<void (
        const TcpConnectionPtr&,
        const muduo::string &message,
                muduo::Timestamp
        )>
        OnMessageCallback;

        typedef std::function<void (
        const TcpConnectionPtr&
        )>
        OnWriteCompleteCallback;

        GatewayServer(EventLoop *loop,
                      const InetAddress &listenAddr)
                : server_(loop, listenAddr, "GatewayServer"),
                  codec_(std::bind(&GatewayServer::onConnection, this, _1),
                         std::bind(&GatewayServer::onMessage, this, _1, _2, _3),
                         std::bind(&GatewayServer::onWriteComplete, this, _1)) {
            server_.setConnectionCallback(
                    std::bind(&WebSocketCodec::onConnection, &codec_, _1));
            server_.setMessageCallback(
                    std::bind(&WebSocketCodec::onMessage, &codec_, _1, _2, _3));
            server_.setWriteCompleteCallback(
                    std::bind(&WebSocketCodec::onWriteComplete, &codec_, _1));
        }

        void setThreadNum(int numThreads) {
            server_.setThreadNum(numThreads);
        }

        void start() {
            server_.setThreadInitCallback(std::bind(&GatewayServer::threadInit, this, _1));
            server_.start();
        }

        void setOnConnectionCallback(const OnConnectionCallback &cb) {
            onConnection_ = cb;
        }

        void setOnMessageCallback(const OnMessageCallback &cb) {
            onMessage_ = cb;
        }

        void setOnWriteCompleteCallback(const OnWriteCompleteCallback &cb) {
            onWriteComplete_ = cb;
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
                LocalClient::instance()[conn] = id;
            } else {
                LocalConnections::instance().erase(id);
                LocalClient::instance().erase(conn);
            }

            onConnection_(conn);
        }

        void onMessage(const TcpConnectionPtr &conn,
                       const string &message,
                       Timestamp timestamp) {
            LOG_INFO << conn->localAddress().toIpPort() << " -> "
                     << conn->peerAddress().toIpPort() << " -> "
                     << "message" << " -> "
                     << message;

            onMessage_(conn, message, timestamp);
        }

        void onWriteComplete(const TcpConnectionPtr &conn) {
            LOG_INFO << conn->localAddress().toIpPort() << " -> "
                     << conn->peerAddress().toIpPort() << " -> "
                     << "COMPLETE";
            onWriteComplete_(conn);
        }

        void distributeMessageAll(const string &message, const std::set<unsigned int> &excludeClientIdList) {
            for (ClientConnectionMap::const_iterator it = LocalConnections::instance().cbegin();
                 it != LocalConnections::instance().cend();
                 ++it) {
                if (excludeClientIdList.find(it->first) == excludeClientIdList.cend()) {
                    codec_.send(get_pointer(it->second), message);
                }
            }
        }

        void distributeMessagePart(const string &message, const std::set<unsigned int> &includeClientIdList) {
            for (ClientConnectionMap::const_iterator it = LocalConnections::instance().cbegin();
                 it != LocalConnections::instance().cend();
                 ++it) {
                if (includeClientIdList.find(it->first) != includeClientIdList.cend()) {
                    codec_.send(get_pointer(it->second), message);
                }
            }
        }

        void distributeMessageOne(const string &message, const unsigned int clientId) {
            ClientConnectionMap::const_iterator it = LocalConnections::instance().find(clientId);
            if (it != LocalConnections::instance().cend()) {
//            codec_.send(get_pointer(it->second), GatewayCodec::GatewayCmd::kGatewayCmdSendToOne,
//                        static_cast<uint32_t>(clientId), message);
                codec_.send(get_pointer(it->second), message);
            }
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
        WebSocketCodec codec_;
        std::set<EventLoop *> loops_;

    private:
        /**
         * keep all client id -> connection
         */
        typedef std::map<unsigned int, TcpConnectionPtr> ClientConnectionMap;

        /**
         * use thread local singleton client id -> connection map
         */
        typedef ThreadLocalSingleton <ClientConnectionMap> LocalConnections;

        /**
         * keep all connection -> client id
         */
        typedef std::map<TcpConnectionPtr, unsigned int> ConnectionClientMap;

        /**
         * use thread local singleton connection -> client id map
         */
        typedef ThreadLocalSingleton <ConnectionClientMap> LocalClient;

        /**
         * index for connection id
         */
        static unsigned int connectionIdIndex_;

        /**
         * generate connection id
         * @return
         */
        unsigned int generateConnectionId();

        /**
         * callback for application
         */
        OnConnectionCallback onConnection_;
        OnMessageCallback onMessage_;
        OnWriteCompleteCallback onWriteComplete_;

    public:

        /**
         * send to all clients
         * @param message
         */
        void sendToAll(const string &message);

        /**
         * send to all except clients
         * @param message
         * @param excludeClientIdList
         */
        void sendToAll(const string &message, const std::set<unsigned int> &excludeClientIdList);

        /**
         * send to all clients except self
         * @param message
         */
        void sendToAllExcludeSelf(const TcpConnectionPtr &conn, const string &message);

        /**
         * send to one client
         * @param clientId
         * @param message
         */
        void sendToClient(unsigned int clientId, const string &message);

        /**
         * send to clients
         * @param message
         * @param includeClientIdList
         */
        void sendToClients(const string &message, const std::set<unsigned int> &includeClientIdList);

        /**
         * conn -> client id
         * @param conn
         * @return
         */
        unsigned int getClientId(const TcpConnectionPtr &conn);
    };
}

#endif //MUDUO_TODPOLE_GATEWAYSERVER_H
