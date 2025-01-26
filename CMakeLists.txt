cmake_minimum_required(VERSION 3.28.3)
project(untitled1)

set(CMAKE_CXX_STANDARD 20)

find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIRS})

add_executable(untitled1 main.cpp)
target_link_libraries(untitled1 ${CURSES_LIBRARIES})
