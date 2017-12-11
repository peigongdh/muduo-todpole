//
// Created by zhangpei-home on 2017/12/7.
//

#include <muduo/base/Logging.h>
#include <muduo/net/EventLoopThread.h>

#include <iostream>

#include <stdio.h>
#include <unistd.h>

#include <todpole/ext/net/gateway/GatewayClient.h>

int main(int argc, char *argv[]) {
    LOG_INFO << "pid = " << getpid();
    if (argc > 2) {
        EventLoopThread loopThread;
        uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
        InetAddress serverAddr(argv[1], port);

        GatewayClient client(loopThread.startLoop(), serverAddr);
        client.connect();
        std::string line;


        while (std::getline(std::cin, line)) {
            // FIXME: use split?
            size_t current;
            size_t next = -1;

            current = next + 1;
            next = line.find_first_of(' ', current);
            int32_t cmd = 0;
            if (next != std::string::npos) {
                cmd = static_cast<int32_t>(std::stoi(line.substr(current, next - current)));
            } else {
                std::cout << "error format" << std::endl;
                continue;
            }

            current = next + 1;
            next = line.find_first_of(' ', current);
            unsigned int ext = 0;
            if (next != std::string::npos) {
                ext = static_cast<unsigned int>(std::stoi(line.substr(current, next - current)));
            } else {
                std::cout << "error format" << std::endl;
                continue;
            }

            current = next + 1;
            next = line.find_first_of(' ', current);
            std::string message;
            if (next == std::string::npos) {
                message = line.substr(current, next - current);
            } else {
                std::cout << "error format" << std::endl;
                continue;
            }

            std::cout << cmd << ext << message << std::endl;

            client.write(static_cast<int32_t>(2), static_cast<unsigned int>(0), "123");
        }
        client.disconnect();
        CurrentThread::sleepUsec(1000 * 1000);  // wait for disconnect, see ace/logging/client.cc
    } else {
        printf("Usage: %s host_ip port\n", argv[0]);
    }
}