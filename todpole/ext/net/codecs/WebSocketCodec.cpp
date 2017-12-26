//
// Created by zhangp on 2017/12/25.
//

#include "todpole/ext/net/codecs/WebSocketCodec.h"

#include "todpole/ext/net/http/HttpContext.h"
#include <muduo/net/http/HttpRequest.h>

namespace muduo {

    namespace net {

        namespace codecs {

            const size_t WebSocketCodec::kMaxSendBufSize = 4096;
            const char *const WebSocketCodec::kSecWebSocketKeyHeader = "Sec-WebSocket-Key";
            const char *const WebSocketCodec::kSecWebSocketProtocolHeader = "Sec-WebSocket-Protocol";

            bool WebSocketCodec::handshake(const TcpConnectionPtr &conn, Buffer *buf, muduo::Timestamp receiveTime) {
                WsConnection *wsConn = std::any_cast<WsConnection>(conn->getMutableContext());
                assert(wsConn);

                if (wsConn->handshaked()) {
                    return true;
                }

                HttpContext context;
                if (!context.parseRequest(buf, receiveTime) || context.request().method() != HttpRequest::kGet) {
                    conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
                    context.reset();
                    return false;
                }

                if (context.gotAll()) {
                    HttpRequest &req = context.request();

                    wsConn->setHandshaked(true);
                    wsConn->setConnState(WS_CONN_OPEN);
                    wsConn->setPath(req.path());
                    wsConn->setProtocol(req.getHeader(kSecWebSocketProtocolHeader));

                    std::string key = req.getHeader(kSecWebSocketKeyHeader);
                    std::string answer = makeHandshakeResponse(key.c_str());

                    conn->send(answer.c_str(), static_cast<int>(answer.size()));
                    context.reset();

                    connectionCallback_(conn);

                    return true;
                }

                return false;
            }
        }
    }
}