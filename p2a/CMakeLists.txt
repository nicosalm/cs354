cmake_minimum_required(VERSION 3.26)
project(p2a)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)
include_directories(given_tests)

add_executable(p2a
        check_board.c)
