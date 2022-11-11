cmake_minimum_required(VERSION 3.4)
project(egraph++)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include)
link_directories(${LLVM_LIBRARY_DIRS})
add_definitions(-fno-rtti)

add_library(EGraph STATIC EGraph.cpp)
