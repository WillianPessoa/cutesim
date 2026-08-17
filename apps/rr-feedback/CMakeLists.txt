# args is a separate static lib so tests can link against it
add_library(rr_feedback_args STATIC args.c args.h display.c display.h)
target_link_libraries(rr_feedback_args PUBLIC cutesim_lib)
target_include_directories(rr_feedback_args PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# MSVC has no <getopt.h>: compile the vendored BSD getopt_long and resolve
# args.c's include from compat/. POSIX builds keep the system getopt.
if(WIN32)
    target_sources(rr_feedback_args PRIVATE compat/getopt_long.c compat/getopt.h)
    target_include_directories(rr_feedback_args PUBLIC compat)
endif()

add_executable(rr-feedback main.c)
# exp() in the Poisson sampler needs libm on Linux (part of libSystem on macOS)
target_link_libraries(rr-feedback PRIVATE rr_feedback_args cutesim_common
    $<$<PLATFORM_ID:Linux>:m>)
