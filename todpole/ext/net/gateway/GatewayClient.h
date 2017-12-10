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

#include "todpole/ext/net/codecs/GatewayCodec.h"

using namespace muduo;
using namespace muduo::net;

using std::placeholders::_4;

class GatewayClient : noncopyable {

public:
    GatewayClient(EventLoop *loop,
                  const InetAddress &listenAddr)
            : client_(loop, listenAddr, "GatewayClient"),
              codec_(std::bind(&GatewayClient::onGatewayMessage, this, _1, _2, _3, _4)) {
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

    void write(const StringPiece &message) {
        MutexLockGuard lock(mutex_);
        if (connection_) {
            codec_.send(get_pointer(connection_), GatewayCodec::GatewayCmd::kGatewayCmdSendToAll, message);
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
                          const int32_t cmd,
                          const string &message,
                          Timestamp) {
        printf("<<< %d: %s\n", cmd, message.c_str());
    }

    TcpClient client_;
    GatewayCodec codec_;
    MutexLock mutex_;
    TcpConnectionPtr connection_;
};

#endif //MUDUO_TODPOLE_GATEWAYCLIENT_H
