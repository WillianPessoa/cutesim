include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
)
FetchContent_MakeAvailable(googletest)
include(GoogleTest)

add_subdirectory(cli)
add_subdirectory(common)
add_subdirectory(queue)
add_subdirectory(process)
add_subdirectory(simulation)
add_subdirectory(statistics)
add_subdirectory(scenario)
add_subdirectory(integration)
add_subdirectory(viewer)
