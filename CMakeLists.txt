cmake_minimum_required(VERSION 3.22)
project(Net_LibEvent)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

add_executable(Net_LibEvent IOCP.cpp)
