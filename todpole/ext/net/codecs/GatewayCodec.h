//
// Created by zhangp on 2017/12/5.
//

#ifndef MUDUO_TODPOLE_GATEWAYCODEC_H
#define MUDUO_TODPOLE_GATEWAYCODEC_H

#include <muduo/base/Logging.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/Endian.h>
#include <muduo/net/TcpConnection.h>

namespace muduo::ext {

    class GatewayCodec : muduo::noncopyable {

    public:
        enum GatewayCmd {
            kGatewayCmdInvalid = 0,
            kGatewayCmdSendToOne = 1,
            kGatewayCmdSendToAll = 2,
        };

        struct GatewayHeader {
            int16_t length;
            int16_t cmd;
            uint32_t ext;
        };

    public:
        typedef std::function<void(
                const muduo::net::TcpConnectionPtr &,
                const int16_t cmd,
                const uint32_t ext,
                const muduo::string &message,
                muduo::Timestamp
        )>
                GatewayMessageCallback;

        explicit GatewayCodec(const GatewayMessageCallback &cb)
                : messageCallback_(cb) {
        }

        void onMessage(const muduo::net::TcpConnectionPtr &conn,
                       muduo::net::Buffer *buf,
                       muduo::Timestamp receiveTime) {
            while (buf->readableBytes() >= kHeaderLen) {
                // FIXME: use Buffer::peekInt32()
                GatewayHeader header;
                memcpy(&header, buf->peek(), kHeaderLen);
                header.length = muduo::net::sockets::networkToHost16(header.length);
                header.cmd = muduo::net::sockets::networkToHost16(header.cmd);
                header.ext = muduo::net::sockets::networkToHost32(header.ext);
                if (header.length < 0) {
                    LOG_ERROR << "Invalid length " << header.length;
                    conn->shutdown();  // FIXME: disable reading
                    break;
                } else if (buf->readableBytes() >= header.length + kHeaderLen) {
                    buf->retrieve(kHeaderLen);
                    muduo::string message(buf->peek(), header.length);
                    messageCallback_(conn, header.cmd, header.ext, message, receiveTime);
                    buf->retrieve(header.length);
                } else {
                    break;
                }
            }
        }

        // FIXME: TcpConnectionPtr
        void send(muduo::net::TcpConnection *conn,
                  const int16_t cmd,
                  const uint32_t ext,
                  const muduo::StringPiece &message) {
            muduo::net::Buffer buf;
            buf.append(message.data(), message.size());
            int16_t length = static_cast<int16_t>(message.size());

            GatewayHeader header;
            header.length = muduo::net::sockets::hostToNetwork16(length);
            header.cmd = muduo::net::sockets::hostToNetwork16(cmd);
            header.ext = muduo::net::sockets::hostToNetwork32(ext);

            buf.prepend(&header, sizeof header);
            conn->send(&buf);
        }

    private:
        GatewayMessageCallback messageCallback_;
        const static size_t kHeaderLen = sizeof(GatewayHeader);
    };
}

#endif //MUDUO_TODPOLE_GATEWAYCODEC_H
