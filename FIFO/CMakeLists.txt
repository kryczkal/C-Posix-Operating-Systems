cmake_minimum_required(VERSION 3.27)
project(Fifo-And-Pipes C)

set(CMAKE_C_STANDARD 11)

include_directories(FIFO)

add_executable(client
        client.c
        client.h)

add_executable(server
        server.c
        server.h)
