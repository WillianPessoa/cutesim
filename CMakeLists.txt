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
    include/cutesim/simulation.h
    include/cutesim/process.h
    include/cutesim/queue.h
    include/cutesim/event.h
    include/cutesim/statistics.h
    include/cutesim/config.h
)
target_include_directories(cutesim_lib PUBLIC include)
target_compile_features(cutesim_lib PRIVATE c_std_11)

include(CTest)

add_subdirectory(apps/rr-feedback)
add_subdirectory(apps/viewer)
add_subdirectory(tests)
