if (NOT MUDUO_TODPOLE_PATH)
    set(MUDUO_TODPOLE_PATH "../../build/${CMAKE_BUILD_TYPE}-install-muduo-todpole")
endif ()

#find_path(TODPOLE_INCLUDE_DIR muduo "${MUDUO_TODPOLE_PATH}/include")
#find_path(TODPOLE_LIBRARY_DIR libmuduo_websocket.a "${MUDUO_TODPOLE_PATH}/lib")
#
#set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${TODPOLE_LIBRARY_DIR})
#
#message(STATUS ${TODPOLE_INCLUDE_DIR})
#message(STATUS ${TODPOLE_LIBRARY_DIR})
#
#include_directories(${TODPOLE_INCLUDE_DIR})

set(base_SRCS
        base/Base64.cpp
        base/StringUtil.cpp
        base/SHA1.cpp
        )
add_library(muduo_ext_base ${base_SRCS})
target_link_libraries(muduo_ext_base)
install(TARGETS muduo_ext_base DESTINATION lib)

set(base_HEADERS
        base/Base64.h
        base/StringUtil.h
        base/SHA1.h
        )
install(FILES ${base_HEADERS} DESTINATION include/muduo/ext/base)



set(http_SRCS
        net/http/HttpContext.cc
        )
add_library(muduo_ext_http ${http_SRCS})
target_link_libraries(muduo_ext_http muduo_net muduo_base muduo_http)
install(TARGETS muduo_ext_http DESTINATION lib)

set(http_HEADERS
        net/http/HttpContext.cc
        )
install(FILES ${http_HEADERS} DESTINATION include/muduo/ext/net/http)



set(HEADERS
        net/websocket/WebSocket.cpp
        # net/websocket/WebSocketClient.cpp
        net/websocket/WebSocketServer.cpp
        )
add_library(muduo_ext_websocket ${HEADERS})
target_link_libraries(muduo_ext_websocket muduo_net muduo_base muduo_http muduo_ext_base muduo_ext_http)
install(TARGETS muduo_ext_websocket DESTINATION lib)

set(websocket_HEADERS
        net/websocket/WebSocket.h
        # net/websocket/WebSocketClient.h
        net/websocket/WebSocketServer.h
        )
install(FILES ${websocket_HEADERS} DESTINATION include/muduo/ext/net/websocket)
