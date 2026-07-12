# args is a separate static lib so tests can link against it
add_library(rr_feedback_args STATIC args.c args.h display.c display.h)
target_link_libraries(rr_feedback_args PUBLIC cutesim_lib)
target_include_directories(rr_feedback_args PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(rr-feedback main.c)
# exp() in the Poisson sampler needs libm on Linux (part of libSystem on macOS)
target_link_libraries(rr-feedback PRIVATE rr_feedback_args cutesim_common
    $<$<PLATFORM_ID:Linux>:m>)
