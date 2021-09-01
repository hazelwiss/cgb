cmake_minimum_required(VERSION 3.10)

project(cgb)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

include_directories("${PROJECT_SOURCE_DIR}")
include_directories("${PROJECT_SOURCE_DIR}/src")

file(GLOB_RECURSE SRC "src/*.c" "src/*.h")

set(C_FLAGS "-g -O0 -Wall")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C_FLAGS}")

add_executable(cgb ${SRC})