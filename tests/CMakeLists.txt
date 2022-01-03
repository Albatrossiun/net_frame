aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} cpps)
add_executable (Test ${cpps})
target_link_libraries (Test
                my_net_thread
                pthread
                )

