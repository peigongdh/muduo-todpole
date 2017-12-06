//
// Created by zhangp on 2017/12/4.
//

#ifndef MUDUO_TODPOLE_GATEWAYCLIENT_H
#define MUDUO_TODPOLE_GATEWAYCLIENT_H

#include <muduo/net/EventLoop.h>
#include <muduo/base/Logging.h>
#include <muduo/base/Mutex.h>
#include <muduo/net/TcpClient.h>

#include <stdio.h>

#include "todpole/ext/net/codecs/LengthHeaderCodec.h"

using namespace muduo;
using namespace muduo::net;

class GatewayClient : noncopyable {

public:
    GatewayClient(EventLoop *loop,
                  const InetAddress &listenAddr)
            : client_(loop, listenAddr, "GatewayClient"),
              codec_(std::bind(&GatewayClient::onStringMessage, this, _1, _2, _3)) {
        client_.setConnectionCallback(
                std::bind(&GatewayClient::onConnection, this, _1));
        client_.setMessageCallback(
                std::bind(&LengthHeaderCodec::onMessage, &codec_, _1, _2, _3));
    }

    void connect() {
        client_.connect();
    }

    void disconnect() {
        client_.disconnect();
    }

    void write(const StringPiece &message) {
        MutexLockGuard lock(mutex_);
        if (connection_) {
            codec_.send(get_pointer(connection_), message);
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

    void onStringMessage(const TcpConnectionPtr &,
                         const string &message,
                         Timestamp) {
        printf("<<< %s\n", message.c_str());
    }

    TcpClient client_;
    LengthHeaderCodec codec_;
    MutexLock mutex_;
    TcpConnectionPtr connection_;
};

#endif //MUDUO_TODPOLE_GATEWAYCLIENT_H
