include(FetchContent)
# Keep GTest on the shared CRT: its default of swapping /MD for /MT on MSVC
# would clash with every other target at link time. No-op elsewhere.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
    SYSTEM
)
# Third-party code is not ours to keep warning-clean: silence warnings while
# GTest's directories are created (-w also mutes AppleClang's default-on
# warnings), then restore our flags for the test subdirectories below.
set_property(DIRECTORY PROPERTY COMPILE_OPTIONS -w)
FetchContent_MakeAvailable(googletest)
set_property(DIRECTORY PROPERTY COMPILE_OPTIONS ${CUTESIM_WARNING_FLAGS})
include(GoogleTest)

add_subdirectory(cli)
add_subdirectory(common)
add_subdirectory(queue)
add_subdirectory(process)
add_subdirectory(simulation)
add_subdirectory(statistics)
add_subdirectory(scenario)
add_subdirectory(integration)
if(CUTESIM_VIEWER)
    add_subdirectory(viewer)
endif()
