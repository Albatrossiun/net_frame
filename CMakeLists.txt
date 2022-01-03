cmake_minimum_required (VERSION 3.1)

add_definitions("-Wall -g")

project (my_net_frame)  

set(ALL_SOURCES CACHE INTERNAL "All sources to be compiled in this project" )

set(CMAKE_CXX_STANDARD 14)

include_directories(SYSTEM ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/src/third/)

add_subdirectory(src)


# 单元测试 
enable_testing()
add_subdirectory(tests)
add_test(NAME MyTest COMMAND Test)

