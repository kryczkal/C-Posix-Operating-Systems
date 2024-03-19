cmake_minimum_required(VERSION 3.28)
project(Client_Server C)

set(CMAKE_C_STANDARD 11)

add_executable(Client
        client.c)
add_executable(Server
        server.c)
