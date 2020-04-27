cmake_minimum_required(VERSION 3.3)
project(skat)

FILE(GLOB dummysources lib/*.c lib/*.h)
add_executable(dummy ${dummysources})
add_custom_target(skat ALL COMMAND make force_rebuild WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
