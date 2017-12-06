//
// Created by zhangp on 2017/12/4.
//

#include "todpole/ext/net/gateway/GatewayServer.h"

#include <climits>

unsigned int GatewayServer::connectionIdIndex_ = 0;

unsigned int GatewayServer::generateConnectionId() {
    MutexLockGuard lock(mutex_);

    // UINT_MAX = 4294967295 on 64bit system
    if (GatewayServer::connectionIdIndex_ >= UINT_MAX) {
        GatewayServer::connectionIdIndex_ = 0;
    }
    while (++GatewayServer::connectionIdIndex_ <= UINT_MAX) {
        if (LocalConnections::instance().find(GatewayServer::connectionIdIndex_) == LocalConnections::instance().end()) {
            break;
        }
        if (GatewayServer::connectionIdIndex_ >= UINT_MAX) {
            GatewayServer::connectionIdIndex_ = 0;
        }
    }
    return GatewayServer::connectionIdIndex_;
}
