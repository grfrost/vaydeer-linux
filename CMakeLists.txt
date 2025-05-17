cmake_minimum_required(VERSION 3.22.1)
project(vaydeer-linux)

set(CMAKE_CXX_STANDARD 14)

add_executable(vaydeer vaydeer.c)
target_link_libraries(vaydeer PUBLIC udev)

