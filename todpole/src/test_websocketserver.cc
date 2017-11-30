#include <iostream>
#include <string>
#include "muduo/net/websocket/WebSocketServer.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/TcpConnection.h"
#include <muduo/base/Timestamp.h>

using namespace std;
using namespace muduo::net;
using namespace muduo::net::ws;

class EchoWebServer
{
public:
    EchoWebServer(EventLoop *loop, const InetAddress& listenAddr)
        : server_(loop, listenAddr, "EchoWebServer")
    {
        server_.setOnOpen(std::bind(&EchoWebServer::onopen, this, std::placeholders::_1));
        server_.setOnClose(std::bind(&EchoWebServer::onclose, this, std::placeholders::_1));
        server_.setOnMessage(std::bind(&EchoWebServer::onmessage, this, std::placeholders::_1,
                                std::placeholders::_2, std::placeholders::_3));
    }

    void start()
    {
        server_.start();
    }

private:
    void onopen(const TcpConnectionPtr& conn)
    {
        assert(conn->connected());
        cout << "EchoWebServer "<< conn->localAddress().toIpPort() << " -> "
                  << conn->peerAddress().toIpPort() << " is "
                  << (conn->connected() ? "UP" : "DOWN") << " connected\n";
    }

    void onclose(const TcpConnectionPtr& conn)
    {
        cout << "EchoWebServer "<< conn->localAddress().toIpPort() << " -> "
             << conn->peerAddress().toIpPort() << " is "
             << (conn->connected() ? "UP" : "DOWN") << " disconnected\n";
    }

    void onmessage(const TcpConnectionPtr& conn, const std::vector<char>& buf, muduo::Timestamp receiveTime)
    {
         cout << "EchoWebServer onmessage : " << buf.data() << "\n";
         //WsConnection *wsconn = zl::stl::any_cast<WsConnection>(conn->getMutableContext());
         //server->sendText(buf.data(), buf.size());
         server_.send(conn, buf.data(), buf.size());
    }

private:
    muduo::net::ws::WsServer server_;
};


int main()
{

    EventLoop loop;
    EchoWebServer server(&loop, InetAddress("0.0.0.0", 8888));

    server.start();
    loop.loop();

    return 0;
}
