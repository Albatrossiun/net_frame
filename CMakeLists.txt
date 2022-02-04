cmake_minimum_required (VERSION 3.1)

add_definitions("-Wall -g")

project (my_net_frame)  

set(ALL_SOURCES CACHE INTERNAL "All sources to be compiled in this project" )

set(CMAKE_CXX_STANDARD 14)

include_directories(SYSTEM ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src/third/)

add_subdirectory(src)

add_executable(my_server server_demo.cc ${ALL_SOURCES})
target_link_libraries(my_server
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

# 单元测试 
enable_testing()
add_subdirectory(tests)
add_test(NAME MyTest COMMAND Test)

