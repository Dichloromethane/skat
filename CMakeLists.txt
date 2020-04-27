cmake_minimum_required(VERSION 3.10)
project(skat)

file(GLOB_RECURSE dummysources src/*.c)
add_executable(dummy ${dummysources})

target_include_directories(dummy PRIVATE include)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(dummy Threads::Threads)

add_custom_target(skat ALL COMMAND make force_rebuild WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
