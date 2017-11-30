#include "todpole/net/websocket/WebSocketServer.h"
// #include "zlreactor/Define.h"
// #include "muduo/base/Logger.h"
#include "muduo/net/TcpConnection.h"
#include "muduo/net/http/HttpRequest.h"
#include "muduo/net/http/HttpResponse.h"
#include "todpole/net/http/HttpContext.h"
#include "todpole/net/websocket/WebSocket.h"

#include <iostream>

namespace muduo
{
namespace net
{
namespace ws
{

const char* const kWebSocketMagicString = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
const char* const kSecWebSocketKeyHeader = "Sec-WebSocket-Key";
const char* const kSecWebSocketVersionHeader = "Sec-WebSocket-Version";
const char* const kUpgradeHeader = "Upgrade";
const char* const kConnectionHeader = "Connection";
const char* const kSecWebSocketProtocolHeader = "Sec-WebSocket-Protocol";
const char* const kSecWebSocketAccept = "Sec-WebSocket-Accept";

WsServer::WsServer(EventLoop *loop, const InetAddress& listenAddr, const string& servername/* = "WsServer"*/)
    : TcpServer(loop, listenAddr, servername)
{
    setConnectionCallback(std::bind(&WsServer::onConnection, this, std::placeholders::_1));
    setMessageCallback(std::bind(&WsServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

WsServer::~WsServer()
{
}

void WsServer::onConnection(const TcpConnectionPtr& conn)
{
    // LOG_INFO("WsServer::onConnection get one client %d", conn->fd());
    if (conn->connected())
    {
        conn->setContext(WsConnection());  // 这里可以用new
    }
    else
    {
        // onClose
    }
}

static void printRequestHeaders(const HttpRequest& req)
{
    std::cout << "---------------print request headers---------------\n";
    std::cout << "Headers " << req.method() << " " << req.path() << std::endl;

    const std::map<string, string> &headers = req.headers();
    for (std::map<string, string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
    {
        std::cout << it->first << ": " << it->second << std::endl;
    }
    std::cout << "---------------------------------------------------\n";
}

void WsServer::onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp receiveTime)
{
    // LOG_INFO("WsServer::onMessage recv data (%d)(size=%d)", conn->fd(), buf->readableBytes());
    WsConnection *wsconn = std::any_cast<WsConnection>(conn->getMutableContext());
    assert(wsconn);
    if(!wsconn->handshaked())
    {
        handshake(conn, buf, receiveTime);
    }
    else
    {
        // LOG_DEBUG("incoming msg: (%d)(%s)", buf->readableBytes(), buf->toString().c_str());
        std::vector<char> outbuf;
        WsFrameType type = decodeFrame(buf->peek(), static_cast<int>(buf->readableBytes()), &outbuf);
        // LOG_DEBUG("getFrame : type=%d, size=%d, data=(%s)", type, outbuf.size(), outbuf.data());
        if(type == WS_INCOMPLETE_TEXT_FRAME || type == WS_INCOMPLETE_BINARY_FRAME)
        {
            return;
        }
        else if(type == WS_TEXT_FRAME || type == WS_BINARY_FRAME)
        {
            buf->retrieveAll();
            onmessage_(conn, outbuf, receiveTime);
        }
        else if(type == WS_CLOSE_FRAME)
        {
            conn->shutdown();
            onclose_(conn);
        }
        else
        {
            // LOG_WARN("No this [%d] opcode handler", type);
        }
    }
}

void WsServer::handshake(const TcpConnectionPtr& conn, Buffer *buf, Timestamp receiveTime)
{
    // LOG_INFO("client[%d] request handshake : \n", conn->fd());
    // const char* over = buf->findDoubleCRLF();
    // FIXME: findDoubleCRLF ?
    const char* over = buf->findCRLF();
    if(!over)   /// 这个地方可能有问题，比如一次性并没有把所有头都发过来。。。
    {
        // LOG_WARN("handshake request data is not ready[%s]", buf->toString().c_str());
        return;
    }

    WsConnection *wsconn = std::any_cast<WsConnection>(conn->getMutableContext());
    assert(wsconn);
    if(wsconn->handshaked())
    {
        return;
    }

    HttpContext context;
    if(!context.parseRequest(buf, receiveTime) || context.request().method() != HttpRequest::kGet)// 解析失败 或者 不是Get请求
    {
        conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
        conn->shutdown();
        return;
    }
    assert(context.gotAll());
    HttpRequest& req = context.request();
    printRequestHeaders(req);
    // LOG_INFO("WsServer::onMessage  parse request over.");

    wsconn->setHandshaked(true);
    wsconn->setConnState(WS_CONN_OPEN);
    wsconn->setPath(req.path());
    wsconn->setProtocol(req.getHeader(kSecWebSocketProtocolHeader));

    std::string key = req.getHeader(kSecWebSocketKeyHeader);
    std::string answer = makeHandshakeResponse(key.c_str());
    // LOG_WARN("client[%d] response handshake : [%s]\n", conn->fd(), answer.c_str());
    conn->send(answer.c_str(), static_cast<int>(answer.size()));

    onopen_(conn);
}

void WsServer::sendText(const TcpConnectionPtr& conn, const char* data, size_t size)
{
    send(conn, data, size, WS_TEXT_FRAME);
}

void WsServer::sendBinary(const TcpConnectionPtr& conn, const char* data, size_t size)
{
    send(conn, data, size, WS_BINARY_FRAME);
}

void WsServer::close(const TcpConnectionPtr& conn, WsCloseReason code, const char* reason)
{
    size_t len = 0;
    char buf[128];
    unsigned short code_be = hton16(code);
    memcpy(buf, &code_be, 2);
    len += 2;
    if (reason)
    {
        len += snprintf(buf + 2, 124, "%s", reason);
    }

    send(conn, buf, len, WS_CLOSE_FRAME);
}

void WsServer::send(const TcpConnectionPtr& conn, const char* data, size_t size, WsFrameType type/* = TEXT_FRAME*/)
{
    const static size_t max_send_buf_size = 4096;
    if(size < max_send_buf_size)
    {
        char outbuf[max_send_buf_size];
        int encodesize = encodeFrame(type, data, static_cast<int>(size), outbuf, max_send_buf_size);
        conn->send(outbuf, encodesize);
    }
    else
    {
        // LOG_DEBUG("client[%d] data big[%d]", conn->fd(), size);
        std::vector<char> outbuf;
        outbuf.resize(size + 10);
        int encodesize = encodeFrame(type, data, static_cast<int>(size), &*outbuf.begin(), outbuf.size());
        conn->send(&*outbuf.begin(), encodesize);
    }
}

}  }  }  // namespace muduo { namespace net { namespace ws {
