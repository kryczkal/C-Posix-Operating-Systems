cmake_minimum_required(VERSION 3.28)
project(Uber_drivers_simulation C)

set(CMAKE_C_STANDARD 11)

add_compile_options(-Wall -Wextra -Werror -Wpedantic)
add_executable(Uber_drivers_simulation
        uber-driver-simulation.c)