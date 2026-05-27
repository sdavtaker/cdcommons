cmake_minimum_required(VERSION 3.28)

# Toolchain must be set before project() so CMake loads vcpkg during compiler detection.
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "")
endif()

project(cdcommons VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_EXTENSIONS OFF)

add_library(cdcommons INTERFACE)
target_include_directories(cdcommons INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
# C++23 expressed as a usage requirement so consumers inherit it correctly.
target_compile_features(cdcommons INTERFACE cxx_std_23)
# Warnings apply only when building from source tree, not when installed via vcpkg.
target_compile_options(cdcommons INTERFACE
    $<BUILD_INTERFACE:-Wall -Wextra -pedantic>
)

# Default to ON only when this is the top-level project; OFF when consumed via add_subdirectory.
option(CDCOMMONS_BUILD_TESTS "Build tests" ${PROJECT_IS_TOP_LEVEL})

if(CDCOMMONS_BUILD_TESTS)
    find_package(Catch2 3 CONFIG REQUIRED)
    enable_testing()
    add_subdirectory(test)
endif()

find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
    file(GLOB_RECURSE ALL_SOURCE_FILES
        CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp"
    )
    add_custom_target(format
        COMMAND ${CLANG_FORMAT} -i ${ALL_SOURCE_FILES}
        COMMENT "Applying clang-format"
    )
    add_custom_target(check-format
        COMMAND ${CLANG_FORMAT} --dry-run --Werror ${ALL_SOURCE_FILES}
        COMMENT "Checking clang-format"
    )
endif()
