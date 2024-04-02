cmake_minimum_required(VERSION 3.29)
project(Monte_Carlo_Pi C)

set(CMAKE_C_STANDARD 11)
add_compile_options(-g -Wall -Wextra -Wpedantic -Werror)

add_executable(Monte_Carlo_Pi
        monte-carlo-pi.c)
