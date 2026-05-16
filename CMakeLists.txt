cmake_minimum_required(VERSION 3.21)

project(
    "cutesim"
    VERSION 0.1.0
    DESCRIPTION "Queue-based process scheduler simulator"
    LANGUAGES C CXX
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

add_library(cutesim_lib STATIC)
target_sources(cutesim_lib PRIVATE
    src/simulation.c
    src/process.c
    src/queue.c
    src/event.c
    src/statistics.c
)
target_include_directories(cutesim_lib PUBLIC include)
target_compile_features(cutesim_lib PRIVATE c_std_11)

find_program(CLANG_FORMAT clang-format)
if(CLANG_FORMAT)
    file(GLOB_RECURSE ALL_SOURCE_FILES
        "${CMAKE_SOURCE_DIR}/src/*.c"
        "${CMAKE_SOURCE_DIR}/include/*.h"
        "${CMAKE_SOURCE_DIR}/apps/*.c"
        "${CMAKE_SOURCE_DIR}/apps/*.cpp"
        "${CMAKE_SOURCE_DIR}/apps/*.h"
        "${CMAKE_SOURCE_DIR}/apps/*.hpp"
        "${CMAKE_SOURCE_DIR}/tests/*.cpp"
    )

    add_custom_target(format
        COMMAND ${CLANG_FORMAT} -i ${ALL_SOURCE_FILES}
        COMMENT "Formatting source files"
    )

    add_custom_target(check-format
        COMMAND ${CLANG_FORMAT} --dry-run --Werror ${ALL_SOURCE_FILES}
        COMMENT "Checking source formatting"
    )
endif()

include(CTest)

add_subdirectory(apps/cli)
add_subdirectory(apps/viewer)
add_subdirectory(tests)
