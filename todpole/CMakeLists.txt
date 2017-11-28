cmake_minimum_required(VERSION 2.6)

project(muduo-todpole)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "debug")
endif ()

if (NOT MUDUO_PATH)
    set(MUDUO_PATH "../build/${CMAKE_BUILD_TYPE}-install-cpp17")
endif ()

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
set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${Muduo_LIBRARY_DIR})
message(STATUS ${Muduo_INCLUDE_DIR})
message(STATUS ${Muduo_LIBRARY_DIR})

include_directories(${Muduo_INCLUDE_DIR})
find_library(muduo_base muduo_base)
find_library(muduo_net muduo_net)
find_library(muduo_http muduo_http)
message(STATUS ${muduo_base})
message(STATUS ${muduo_net})
message(STATUS ${muduo_http})

# find_package(Boost REQUIRED)
# include_directories(${Boost_INCLUDE_DIRS})

include_directories(..)

# echo
add_executable(echo src/echo.cc)
target_link_libraries(echo ${muduo_net})
target_link_libraries(echo ${muduo_base})
target_link_libraries(echo pthread rt)

# http
add_executable(http src/http.cc)
target_link_libraries(http ${muduo_net})
target_link_libraries(http ${muduo_base})
target_link_libraries(http pthread)
target_link_libraries(http ${muduo_http})