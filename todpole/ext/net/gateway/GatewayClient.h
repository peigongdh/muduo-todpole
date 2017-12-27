//
// Created by zhangp on 2017/12/4.
//

#ifndef MUDUO_TODPOLE_GATEWAYCLIENT_H
#define MUDUO_TODPOLE_GATEWAYCLIENT_H

#include <muduo/net/EventLoop.h>
#include <muduo/base/Logging.h>
#include <muduo/base/Mutex.h>
#include <muduo/net/TcpClient.h>

#include "rapidjson/document.h"

#include "todpole/ext/net/codecs/GatewayCodec.h"

#include <iostream>
#include "stdio.h"

using namespace muduo;
using namespace muduo::net;

using namespace rapidjson;

using std::placeholders::_4;
using std::placeholders::_5;

namespace muduo::ext {

    class GatewayClient : noncopyable {

    public:
        GatewayClient(EventLoop *loop,
                      const InetAddress &listenAddr)
                : client_(loop, listenAddr, "GatewayClient"),
                  codec_(std::bind(&GatewayClient::onGatewayMessage, this, _1, _2, _3, _4, _5)) {
            client_.setConnectionCallback(
                    std::bind(&GatewayClient::onConnection, this, _1));
            client_.setMessageCallback(
                    std::bind(&GatewayCodec::onMessage, &codec_, _1, _2, _3));
        }

        void connect() {
            client_.connect();
        }

        void disconnect() {
            client_.disconnect();
        }

        void write(const int16_t cmd, const uint32_t ext, const StringPiece &message) {
            MutexLockGuard lock(mutex_);
            if (connection_) {
                codec_.send(get_pointer(connection_), cmd, ext, message);
            }
        }

    private:
        void onConnection(const TcpConnectionPtr &conn) {
            LOG_INFO << conn->localAddress().toIpPort() << " -> "
                     << conn->peerAddress().toIpPort() << " is "
                     << (conn->connected() ? "UP" : "DOWN");

            MutexLockGuard lock(mutex_);
            if (conn->connected()) {
                connection_ = conn;
            } else {
                connection_.reset();
            }
        }

        void onGatewayMessage(const TcpConnectionPtr &,
                              const int16_t cmd,
                              const uint32_t ext,
                              const string &message,
                              Timestamp) {
            std::cout << "<<< " << "cmd [" << cmd << "] ext [" << ext << "]: " << message << std::endl;

            Document document;
            document.Parse(message.c_str());

            for (Value::ConstMemberIterator itr = document.MemberBegin();
                 itr != document.MemberEnd(); ++itr) {
                std::cout << itr->name.GetString() << " : " << itr->value.GetString() << std::endl;
            }
        }

        TcpClient client_;
        GatewayCodec codec_;
        MutexLock mutex_;
        TcpConnectionPtr connection_;
    };
}

#endif //MUDUO_TODPOLE_GATEWAYCLIENT_H
