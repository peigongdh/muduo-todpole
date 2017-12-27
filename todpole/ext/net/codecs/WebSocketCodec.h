//
// Created by zhangp on 2017/12/25.
//

#ifndef MUDUO_TODPOLE_WEBSOCKETCODEC_H
#define MUDUO_TODPOLE_WEBSOCKETCODEC_H

#include <muduo/net/TcpConnection.h>
#include <muduo/net/Buffer.h>

#include "todpole/ext/net/websocket/WebSocket.h"

using namespace muduo::net;
using namespace muduo::net::ws;

namespace muduo::ext {

    class WebSocketCodec : muduo::noncopyable {

    public:

        typedef std::function<void (
        const TcpConnectionPtr&
        )>
        WebSocketConnectionCallback;

        typedef std::function<void (
        const TcpConnectionPtr&,
        const muduo::string &message,
                muduo::Timestamp
        )>
        WebSocketMessageCallback;

        typedef std::function<void (
        const TcpConnectionPtr&
        )>
        WebSocketCloseCallback;

        explicit WebSocketCodec(const WebSocketConnectionCallback &connectionCb,
                                const WebSocketMessageCallback &messageCb,
                                const WebSocketCloseCallback &closeCb)
                : connectionCallback_(connectionCb), messageCallback_(messageCb), closeCallback_(closeCb) {
        }

        void onConnection(const TcpConnectionPtr &conn) {
            if (conn->connected()) {
                conn->setContext(WsConnection());
            } else {
                // FIXME: onClose
            }
            // do callback after handshake
            // connectionCallback_(conn);
        }

        void onMessage(const TcpConnectionPtr &conn, Buffer *buf, muduo::Timestamp receiveTime) {
            WsConnection *wsConn = std::any_cast<WsConnection>(conn->getMutableContext());
            assert(wsConn);
            if (!wsConn->handshaked()) {
                handshake(conn, buf, receiveTime);
            } else {
                std::vector<char> outBuf;
                WsFrameType type = decodeFrame(buf->peek(), static_cast<int>(buf->readableBytes()), &outBuf);
                if (type == WS_INCOMPLETE_TEXT_FRAME || type == WS_INCOMPLETE_BINARY_FRAME) {
                    return;
                } else if (type == WS_TEXT_FRAME || type == WS_BINARY_FRAME) {
                    buf->retrieveAll();
                    // FIXME: not use string
                    muduo::string message(outBuf.begin(), outBuf.end());
                    messageCallback_(conn, message, receiveTime);
                } else if (type == WS_CLOSE_FRAME) {
                    conn->shutdown();
                    closeCallback_(conn);
                } else {
                    // LOG_WARN("No this [%d] opcode handler", type);
                }
            }
        }

        void onClose(const TcpConnectionPtr &conn) {
            // FIXME: onClose
            closeCallback_(conn);
        }

        void send(muduo::net::TcpConnection *conn,
                  const muduo::StringPiece &message) {
            const char *data = message.data();
            size_t size = message.size();
            WsFrameType type = WS_TEXT_FRAME;

            if (size < kMaxSendBufSize) {
                char outBuf[kMaxSendBufSize];
                int encodeSize = encodeFrame(type, data, static_cast<int>(size), outBuf, kMaxSendBufSize);
                conn->send(outBuf, encodeSize);
            } else {
                std::vector<char> outBuf;
                outBuf.resize(size + 10);
                int encodeSize = encodeFrame(type, data, static_cast<int>(size), &*outBuf.begin(),
                                             outBuf.size());
                conn->send(&*outBuf.begin(), encodeSize);
            }
        }

    private:

        const static size_t kMaxSendBufSize;
        const static char *const kSecWebSocketKeyHeader;
        const static char *const kSecWebSocketProtocolHeader;

        WebSocketConnectionCallback connectionCallback_;
        WebSocketMessageCallback messageCallback_;
        WebSocketCloseCallback closeCallback_;

        /**
         * handshake use http protocol
         */
        bool handshake(const TcpConnectionPtr &conn, Buffer *buf, muduo::Timestamp receiveTime);

    };
}

#endif //MUDUO_TODPOLE_WEBSOCKETCODEC_H
