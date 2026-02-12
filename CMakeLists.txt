cmake_minimum_required(VERSION 3.20)
project(almost VERSION 0.1.0 LANGUAGES CXX)

add_library(almost INTERFACE)

add_library(almost::almost ALIAS almost)

target_compile_features(almost INTERFACE cxx_std_23)
target_include_directories(almost
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

option(ALMOST_BUILD_TESTS "Build tests" ON)
if(ALMOST_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
