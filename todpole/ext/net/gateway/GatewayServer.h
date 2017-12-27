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
        GatewayServer(EventLoop *loop,
                      const InetAddress &listenAddr)
                : server_(loop, listenAddr, "GatewayServer"),
                  codec_(std::bind(&GatewayServer::onConnection, this, _1),
                         std::bind(&GatewayServer::onMessage, this, _1, _2, _3),
                         std::bind(&GatewayServer::onClose, this, _1)) {
            server_.setConnectionCallback(
                    std::bind(&WebSocketCodec::onConnection, &codec_, _1));
            server_.setMessageCallback(
                    std::bind(&WebSocketCodec::onMessage, &codec_, _1, _2, _3));
            server_.setWriteCompleteCallback(
                    std::bind(&WebSocketCodec::onClose, &codec_, _1));
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

        void onMessage(const TcpConnectionPtr &,
                       const string &message,
                       Timestamp) {
//        switch (cmd) {
//            case GatewayCodec::GatewayCmd::kGatewayCmdInvalid:
//                // TODO:
//                break;
//            case GatewayCodec::GatewayCmd::kGatewayCmdSendToOne:
//                this->sendToClient(static_cast<unsigned int>(ext), message);
//                break;
//            case GatewayCodec::GatewayCmd::kGatewayCmdSendToAll:
//                this->sendToAll(message);
//                break;
//            default:
//                // FIXME:
//                break;
//        }
            LOG_INFO << message;
        }

        void onClose(const TcpConnectionPtr &conn) {
            LOG_INFO << conn->localAddress().toIpPort() << " -> "
                     << conn->peerAddress().toIpPort() << " -> "
                     << "CLOSE";
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
         * send to all except clients
         * @param message
         * @param excludeClientIdList
         */
        void sendToAll(const string &message, const std::set<unsigned int> &excludeClientIdList);

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
    };
}

#endif //MUDUO_TODPOLE_GATEWAYSERVER_H
