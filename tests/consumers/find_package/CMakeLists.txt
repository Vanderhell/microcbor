cmake_minimum_required(VERSION 3.16)
project(microcbor_find_package_consumer LANGUAGES C)

find_package(microcbor CONFIG REQUIRED)

add_executable(find_package_consumer main.c)
target_link_libraries(find_package_consumer PRIVATE microcbor::microcbor)
