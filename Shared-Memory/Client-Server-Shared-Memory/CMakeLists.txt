cmake_minimum_required(VERSION 3.29)
project(Client_Server_Shared_Memory C)

set(CMAKE_C_STANDARD 11)

add_compile_options(-Wall -Wextra -Werror -Wpedantic)
add_executable(Server
        server.c)
add_executable(Client
        client.c)
