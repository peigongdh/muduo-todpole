cmake_minimum_required(VERSION 2.6)

project(muduo-todpole)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "debug")
endif ()

if (NOT MUDUO_PATH)
    set(MUDUO_PATH "../build/${CMAKE_BUILD_TYPE}-install-cpp17")
endif ()

#if (NOT MUDUO_TODPOLE_PATH)
#    set(MUDUO_TODPOLE_PATH "../build/${CMAKE_BUILD_TYPE}-install-muduo-todpole")
#endif ()

set(CXX_FLAGS
        -g
        # -DVALGRIND
        -DMUDUO_STD_STRING
        -DCHECK_PTHREAD_RETURN_VALUE
        -D_FILE_OFFSET_BITS=64
        -Wall
        -Wextra
        -Werror
        -Wconversion
        -Wno-unused-parameter
        -Wold-style-cast
        -Woverloaded-virtual
        -Wpointer-arith
        -Wshadow
        -Wwrite-strings
        -march=native
        # -MMD
        -std=c++17
        -rdynamic
        )
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -finline-limit=1000 -DNDEBUG")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

find_path(Muduo_INCLUDE_DIR muduo "${MUDUO_PATH}/include")
find_path(Muduo_LIBRARY_DIR libmuduo_net.a "${MUDUO_PATH}/lib")

#find_path(TODPOLE_INCLUDE_DIR muduo "${MUDUO_TODPOLE_PATH}/include")
#find_path(TODPOLE_LIBRARY_DIR libmuduo_websocket.a "${MUDUO_TODPOLE_PATH}/lib")

# set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${Muduo_LIBRARY_DIR} ${TODPOLE_LIBRARY_DIR})
set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${Muduo_LIBRARY_DIR})

message(STATUS ${Muduo_INCLUDE_DIR})
message(STATUS ${Muduo_LIBRARY_DIR})

#message(STATUS ${TODPOLE_INCLUDE_DIR})
#message(STATUS ${TODPOLE_LIBRARY_DIR})

include_directories(${Muduo_INCLUDE_DIR})
# include_directories(${TODPOLE_INCLUDE_DIR})

find_library(muduo_base muduo_base)
find_library(muduo_net muduo_net)
find_library(muduo_http muduo_http)
message(STATUS ${muduo_base})
message(STATUS ${muduo_net})
message(STATUS ${muduo_http})

include_directories(..)

# websocket
#set(websocket_SRCS
#        net/websocket/WebSocket.cpp
#        # net/websocket/WebSocketClient.cpp
#        net/websocket/WebSocketServer.cpp
#        )
#add_library(muduo_websocket ${websocket_SRCS})
#target_link_libraries(muduo_websocket ${muduo_net} ${muduo_base} ${muduo_http})
#
#install(TARGETS muduo_websocket DESTINATION lib)
#set(websocket_HEADERS
#        net/websocket/WebSocket.h
#        # net/websocket/WebSocketClient.h
#        net/websocket/WebSocketServer.h
#        )
#install(FILES ${websocket_HEADERS} DESTINATION include/muduo/net/websocket)

# echo
add_executable(echo src/echo.cc)
target_link_libraries(echo ${muduo_net} ${muduo_base} pthread rt)

# simple_http
add_executable(simple_http src/simple_http.cc)
target_link_libraries(simple_http ${muduo_net} ${muduo_base} ${muduo_http} pthread)

# test_websocketserver
find_library(muduo_websocket muduo_websocket)
message(STATUS ${muduo_websocket})

add_executable(test_websocketserver src/test_websocketserver.cc)
target_link_libraries(test_websocketserver ${muduo_net} ${muduo_base} ${muduo_websocket} pthread)