cmake_minimum_required(VERSION 3.3)
project(skat)

add_custom_target(skat ALL COMMAND make -j1 force_rebuild WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
