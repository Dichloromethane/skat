cmake_minimum_required(VERSION 3.10)
project(skat)

file(GLOB_RECURSE dummysources src/*.c src/*.h)
add_executable(dummy ${dummysources})
target_include_directories(dummy PRIVATE conf)
target_include_directories(dummy PRIVATE include)
target_include_directories(dummy PRIVATE /usr/include/freetype2) #/usr/include/libpng16
target_link_libraries(dummy m pthread glfw GL dl freetype)

add_custom_target(skat_server ALL COMMAND make force_rebuild WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_custom_target(skat_client ALL COMMAND make force_rebuild WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
