//
// Created by zhangp on 2017/12/4.
//

#include "todpole/ext/net/gateway/GatewayServer.h"

#include <climits>

using namespace muduo::ext;

unsigned int GatewayServer::connectionIdIndex_ = 0;

void GatewayServer::sendToAll(const string &message) {
    EventLoop::Functor f = std::bind(&GatewayServer::distributeMessageAll, this, message, std::set<unsigned int>());
    LOG_DEBUG;

    MutexLockGuard lock(mutex_);
    for (std::set<EventLoop *>::iterator it = loops_.begin();
         it != loops_.end();
         ++it) {
        (*it)->queueInLoop(f);
    }
    LOG_DEBUG;
}

void GatewayServer::sendToAll(const string &message, const std::set<unsigned int> &excludeClientIdList) {
    EventLoop::Functor f = std::bind(&GatewayServer::distributeMessageAll, this, message, excludeClientIdList);
    LOG_DEBUG;

    MutexLockGuard lock(mutex_);
    for (std::set<EventLoop *>::iterator it = loops_.begin();
         it != loops_.end();
         ++it) {
        (*it)->queueInLoop(f);
    }
    LOG_DEBUG;
}

void GatewayServer::sendToClient(const unsigned int clientId, const string &message) {
    EventLoop::Functor f = std::bind(&GatewayServer::distributeMessageOne, this, message, clientId);
    LOG_DEBUG;

    MutexLockGuard lock(mutex_);
    for (std::set<EventLoop *>::iterator it = loops_.begin();
         it != loops_.end();
         ++it) {
        (*it)->queueInLoop(f);
    }
    LOG_DEBUG;
}

void GatewayServer::sendToClients(const string &message, const std::set<unsigned int> &includeClientIdList) {
    EventLoop::Functor f = std::bind(&GatewayServer::distributeMessagePart, this, message, includeClientIdList);
    LOG_DEBUG;

    MutexLockGuard lock(mutex_);
    for (std::set<EventLoop *>::iterator it = loops_.begin();
         it != loops_.end();
         ++it) {
        (*it)->queueInLoop(f);
    }
    LOG_DEBUG;
}

unsigned int GatewayServer::generateConnectionId() {
    MutexLockGuard lock(mutex_);
    // UINT_MAX = 4294967295 on 64bit system
    if (GatewayServer::connectionIdIndex_ >= UINT_MAX) {
        GatewayServer::connectionIdIndex_ = 0;
    }
    while (++GatewayServer::connectionIdIndex_ <= UINT_MAX) {
        if (LocalConnections::instance().find(GatewayServer::connectionIdIndex_) ==
            LocalConnections::instance().end()) {
            break;
        }
        if (GatewayServer::connectionIdIndex_ >= UINT_MAX) {
            GatewayServer::connectionIdIndex_ = 0;
        }
    }
    return GatewayServer::connectionIdIndex_;
}
