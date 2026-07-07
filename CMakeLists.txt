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
    src/rng.c
    src/scenario.c
    src/statistics.c
    include/cutesim/simulation.h
    include/cutesim/process.h
    include/cutesim/queue.h
    include/cutesim/rng.h
    include/cutesim/scenario.h
    include/cutesim/statistics.h
    include/cutesim/config.h
)
target_include_directories(cutesim_lib PUBLIC include)
target_compile_features(cutesim_lib PRIVATE c_std_11)

include(CTest)

add_subdirectory(apps/common)
add_subdirectory(apps/rr-feedback)
add_subdirectory(apps/viewer)
add_subdirectory(tests)

# `format` rewrites every C/C++ and QML source in place. The pre-commit hook
# (scripts/pre-commit) only checks and points here when a file needs it.
find_program(CLANG_FORMAT_PROGRAM clang-format)
find_program(QMLFORMAT_PROGRAM qmlformat HINTS "${Qt6_DIR}/../../../bin")

file(GLOB_RECURSE FORMAT_C_SOURCES
    ${CMAKE_SOURCE_DIR}/src/*.c
    ${CMAKE_SOURCE_DIR}/include/*.h
    ${CMAKE_SOURCE_DIR}/apps/*.c
    ${CMAKE_SOURCE_DIR}/apps/*.h
    ${CMAKE_SOURCE_DIR}/apps/*.cpp
    ${CMAKE_SOURCE_DIR}/tests/*.cpp
    ${CMAKE_SOURCE_DIR}/tests/*.h
)
file(GLOB_RECURSE FORMAT_QML_SOURCES
    ${CMAKE_SOURCE_DIR}/apps/*.qml
    ${CMAKE_SOURCE_DIR}/tests/*.qml
)

set(FORMAT_COMMANDS "")
if(CLANG_FORMAT_PROGRAM)
    list(APPEND FORMAT_COMMANDS COMMAND ${CLANG_FORMAT_PROGRAM} -i ${FORMAT_C_SOURCES})
endif()
if(QMLFORMAT_PROGRAM)
    list(APPEND FORMAT_COMMANDS COMMAND ${QMLFORMAT_PROGRAM} -i ${FORMAT_QML_SOURCES})
endif()
if(FORMAT_COMMANDS)
    add_custom_target(format
        ${FORMAT_COMMANDS}
        COMMENT "Formatting C/C++ and QML sources in place"
        VERBATIM
    )
endif()
