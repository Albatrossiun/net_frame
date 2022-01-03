aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} cpps)

add_library (net_frame_net_socket OBJECT ${cpps})
