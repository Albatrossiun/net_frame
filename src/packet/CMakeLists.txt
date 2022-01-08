aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} cpps)

add_library (my_net_packet OBJECT ${cpps})
