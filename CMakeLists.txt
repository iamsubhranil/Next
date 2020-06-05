cmake_minimum_required(VERSION 3.14)
project(Next)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

file(GLOB sources *.cpp objects/*.cpp)

add_executable(next ${sources})

add_executable(next_cov ${sources})
target_compile_options(next_cov PUBLIC "-fprofile-arcs" "-ftest-coverage")
target_link_options(next_cov PUBLIC "-fprofile-arcs" "-ftest-coverage")