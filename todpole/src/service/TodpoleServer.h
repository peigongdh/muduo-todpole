//
// Created by zhangp on 2017/12/28.
//

#ifndef MUDUO_TODPOLE_TODPOLESERVER_H
#define MUDUO_TODPOLE_TODPOLESERVER_H

#include <muduo/net/EventLoop.h>
#include <muduo/base/Logging.h>
#include <todpole/ext/net/gateway/GatewayServer.h>

using namespace muduo;
using namespace muduo::ext;

class TodpoleServer : noncopyable {

public:

    TodpoleServer(EventLoop *loop,
                  const InetAddress &listenAddr)
            : server_(loop, listenAddr) {
        server_.setOnConnectionCallback(
                std::bind(&TodpoleServer::onConnection, this, _1));
        server_.setOnMessageCallback(
                std::bind(&TodpoleServer::onMessage, this, _1, _2, _3));
        server_.setOnWriteCompleteCallback(
                std::bind(&TodpoleServer::onWriteComplete, this, _1));
    }

    void onConnection(const TcpConnectionPtr &conn) {
    }

    void onMessage(const TcpConnectionPtr &conn,
                   const string &message,
                   Timestamp timestamp);

    void onWriteComplete(const TcpConnectionPtr &conn) {
    }

    void setThreadNum(int numThreads) {
        server_.setThreadNum(numThreads);
    }

    void start() {
        server_.start();
    }

private:

    GatewayServer server_;

};

#endif //MUDUO_TODPOLE_TODPOLESERVER_H
