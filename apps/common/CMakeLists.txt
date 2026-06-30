add_library(cutesim_common STATIC snapshot.c snapshot.h)
target_link_libraries(cutesim_common PUBLIC cutesim_lib)
target_include_directories(cutesim_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
