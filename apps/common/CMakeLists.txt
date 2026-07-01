find_package(Threads REQUIRED)

add_library(cutesim_common STATIC
    snapshot.c snapshot.h
    emit_file.c emit_file.h
    emit_tcp.c emit_tcp.h)
target_link_libraries(cutesim_common PUBLIC cutesim_lib Threads::Threads)
target_include_directories(cutesim_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
