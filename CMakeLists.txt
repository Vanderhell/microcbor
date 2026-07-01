cmake_minimum_required(VERSION 3.16)

project(microcbor VERSION 1.0.0 LANGUAGES C)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

option(MCBOR_BUILD_TESTS "Build microcbor tests" ON)
option(MCBOR_BUILD_FUZZ "Build microcbor fuzz targets" OFF)
option(MCBOR_ENABLE_FLOAT32 "Enable float32 encode/decode support" ON)
option(MCBOR_STRICT_WARNINGS "Enable strict warnings for microcbor-owned targets" ON)

set(MCBOR_GENERATED_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/include")
set(MCBOR_ENABLE_FLOAT32_VALUE 0)
if(MCBOR_ENABLE_FLOAT32)
    set(MCBOR_ENABLE_FLOAT32_VALUE 1)
endif()
file(MAKE_DIRECTORY "${MCBOR_GENERATED_INCLUDE_DIR}")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/mcbor_config.h.in"
    "${MCBOR_GENERATED_INCLUDE_DIR}/mcbor_config.h"
    @ONLY
)

add_library(microcbor src/mcbor.c)
add_library(microcbor::microcbor ALIAS microcbor)

target_compile_features(microcbor PUBLIC c_std_99)
target_include_directories(microcbor
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${MCBOR_GENERATED_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_definitions(microcbor PUBLIC
    MCBOR_ENABLE_FLOAT32=$<IF:$<BOOL:${MCBOR_ENABLE_FLOAT32}>,1,0>
)

if(MCBOR_STRICT_WARNINGS)
    if(MSVC)
        target_compile_options(microcbor PRIVATE /W4 /WX)
    else()
        target_compile_options(microcbor PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
endif()

if(MCBOR_BUILD_TESTS)
    add_executable(microcbor_test tests/test_all.c)
    target_link_libraries(microcbor_test PRIVATE microcbor)
    if(NOT MSVC)
        target_link_libraries(microcbor_test PRIVATE m)
    endif()
endif()

if(MCBOR_BUILD_FUZZ)
    add_executable(microcbor_fuzz tests/fuzz/fuzz_decode.c)
    target_link_libraries(microcbor_fuzz PRIVATE microcbor)
endif()

install(TARGETS microcbor
    EXPORT microcborTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(FILES
    include/mcbor.h
    "${MCBOR_GENERATED_INCLUDE_DIR}/mcbor_config.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

install(EXPORT microcborTargets
    FILE microcborTargets.cmake
    NAMESPACE microcbor::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/microcbor"
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/microcborConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/microcborConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/microcborConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/microcbor"
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/microcborConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/microcborConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/microcbor"
)
