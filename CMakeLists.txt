cmake_minimum_required(VERSION 4.1)
project(RemoteSystemMonitor)

set(CMAKE_CXX_STANDARD 14)

add_executable(RemoteSystemMonitor
        Client/main.cpp
        Server/main.cpp)

target_link_libraries(RemoteSystemMonitor pdh ws2_32)