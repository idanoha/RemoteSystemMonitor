cmake_minimum_required(VERSION 4.1)
project(RemoteSystemMonitor)

set(CMAKE_CXX_STANDARD 17)

add_executable(Server Server/main.cpp)
target_link_libraries(Server PRIVATE pdh ws2_32)

add_executable(Client Client/main.cpp)
target_link_libraries(Client PRIVATE ws2_32)