cmake_minimum_required(VERSION 3.5)
project(Messenger LANGUAGES CXX)

set(SRC_DIR src)

add_subdirectory(${SRC_DIR}/Client)
add_subdirectory(${SRC_DIR}/Server)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/ssl/ DESTINATION ${CMAKE_BINARY_DIR}/bin/ssl)