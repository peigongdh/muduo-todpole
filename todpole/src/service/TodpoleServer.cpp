//
// Created by zhangp on 2017/12/28.
//

#include "todpole/src/service/TodpoleServer.h"
#include "rapidjson/document.h"

using namespace rapidjson;

void TodpoleServer::onMessage(const TcpConnectionPtr &conn,
                              const string &message,
                              Timestamp timestamp) {

    Document document;
    document.Parse(message.c_str());
    assert(document.IsObject());
    assert(document.HasMember("type"));
    assert(document["type"].IsString());

    string type = document["type"].GetString();

    if (type == "shoot") {
        server_.sendToAll(message);
    } else {
        // TODO
    }
}
