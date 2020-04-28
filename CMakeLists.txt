cmake_minimum_required(VERSION 3.10)
project(skat)

file(GLOB_RECURSE dummysources src/*.c)
add_executable(dummy ${dummysources})
target_include_directories(dummy PRIVATE include)

add_custom_target(skat_server ALL COMMAND make skat_server WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_custom_target(skat_client ALL COMMAND make skat_client WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
