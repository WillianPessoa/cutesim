add_executable(cutesim_cli)
target_sources(cutesim_cli PRIVATE main.c)
target_link_libraries(cutesim_cli PRIVATE cutesim_lib)
