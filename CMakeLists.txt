cmake_minimum_required(VERSION 3.0)
project(lua_test)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g ")

include(CheckCXXCompilerFlag)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")

include_directories(lua-4.0.1/include lua-4.0.1/src test .)

aux_source_directory(lua-4.0.1/src common_src)        # lua4的主要源码
aux_source_directory(lua-4.0.1/src/lib common_src)    # lua4的库源码

add_library(lua4lib ${common_src})

enable_testing()
add_subdirectory(test)