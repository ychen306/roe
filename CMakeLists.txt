cmake_minimum_required(VERSION 3.4)
project(egraph++)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

###### GET GTEST ######
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt
    ON
    CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
######################################

find_package(LLVM REQUIRED CONFIG)
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include)
link_directories(${LLVM_LIBRARY_DIRS})
add_definitions(-fno-rtti -fvisibility=hidden)

add_library(EGraph STATIC EGraph.cpp)

include(GoogleTest)
add_executable(tests tests.cpp)
llvm_map_components_to_libnames(LLVM_LIBS support)
target_link_libraries(tests gtest_main EGraph ${LLVM_LIBS})
gtest_add_tests(TARGET tests)
