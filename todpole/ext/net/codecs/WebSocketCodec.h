//
// Created by zhangp on 2017/12/25.
//

#ifndef MUDUO_TODPOLE_WEBSOCKETCODEC_H
#define MUDUO_TODPOLE_WEBSOCKETCODEC_H

class WebSocketCodec : muduo::noncopyable {

public:

    typedef std::function<void (
    const muduo::net::TcpConnectionPtr&
    )>
    WebSocketConnectionCallback;

    typedef std::function<void (
    const muduo::net::TcpConnectionPtr&,
    const muduo::string &message,
            muduo::Timestamp
    )>
    WebSocketMessageCallback;

    explicit WebSocketCodec(const WebSocketConnectionCallback &cCb, const WebSocketMessageCallback &mCb)
            : connectionCallback_(cCb), messageCallback_(mCb) {
    }

    void onConnection(const TcpConnectionPtr &conn) {
        connectionCallback_(conn);
    }

    void onMessage(const muduo::net::TcpConnectionPtr &conn,
                   muduo::net::Buffer *buf,
                   muduo::Timestamp receiveTime) {

    }

private:
    WebSocketConnectionCallback connectionCallback_;
    WebSocketMessageCallback messageCallback_;

};

#endif //MUDUO_TODPOLE_WEBSOCKETCODEC_H
