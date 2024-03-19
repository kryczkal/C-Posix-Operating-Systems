cmake_minimum_required(VERSION 3.28)
project(Posix_message_queues C)

set(CMAKE_C_STANDARD 11)
#add_compile_options(-Wall -Wextra -Werror -Wpedantic -lrt)
add_compile_options(-Wall -Wextra -Werror -Wpedantic)

add_executable(Posix_message_queues
        bingo-simulation.c)
