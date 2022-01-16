aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} cpps)
add_executable (Test ${cpps})
target_link_libraries (Test
                my_net_thread
                my_net_handler
                my_net_util_datablob
                my_net_util_time
                my_net_util_tools
                my_net_socket
                my_net_io_loop
                pthread
                my_net_packet
                my_net_third_http_parser
                my_net_user
                )

