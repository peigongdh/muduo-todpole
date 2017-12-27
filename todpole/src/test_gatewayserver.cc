//
// Created by zhangp on 2017/12/5.
//

#include <muduo/base/Logging.h>
#include <muduo/net/EventLoop.h>

#include <stdio.h>
#include <unistd.h>

#include <todpole/ext/net/gateway/GatewayServer.h>

int main(int argc, char *argv[]) {
    LOG_INFO << "pid = " << getpid();
    if (argc > 1) {
        EventLoop loop;
        uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
        InetAddress serverAddr(port);
        muduo::ext::GatewayServer gateway(&loop, serverAddr);
        if (argc > 2) {
            gateway.setThreadNum(atoi(argv[2]));
        }
        gateway.start();
        loop.loop();
    } else {
        printf("Usage: %s port [thread_num]\n", argv[0]);
    }
}