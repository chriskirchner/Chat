
project(chatclient)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES chatclient.cpp)
add_executable(chatclient ${SOURCE_FILES})
