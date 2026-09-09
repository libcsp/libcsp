cmake_minimum_required(VERSION 3.20)
project(csp_cpp_consumer LANGUAGES CXX)

add_subdirectory(../.. libcsp)

add_executable(csp_cpp_consumer main.cpp)
target_compile_features(csp_cpp_consumer PRIVATE cxx_std_11)
target_link_libraries(csp_cpp_consumer PRIVATE csp)
