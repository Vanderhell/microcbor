cmake_minimum_required(VERSION 3.16)
project(microcbor_add_subdirectory_consumer LANGUAGES C CXX)

add_subdirectory("../../.." microcbor-build)

add_executable(add_subdirectory_consumer main.c)
target_link_libraries(add_subdirectory_consumer PRIVATE microcbor::microcbor)

add_executable(cxx_header_consumer main.cpp)
target_link_libraries(cxx_header_consumer PRIVATE microcbor::microcbor)
