cmake_minimum_required(VERSION 3.5)
project(Messenger LANGUAGES CXX)

add_subdirectory(Client)
add_subdirectory(Server)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/ssl/ DESTINATION ${CMAKE_BINARY_DIR}/bin/ssl)