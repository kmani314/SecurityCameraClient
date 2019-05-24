# Security Camera Client
# By Krishna Mani on 5/23/2019

cmake_minimum_required(VERSION 2.9)
project(client)

file(GLOB_RECURSE SOURCES src/*.cpp)

# Output to bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/)

add_executable(client ${SOURCES})

include_directories(client include/)
