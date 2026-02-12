include(FetchContent)

set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/32c3dce34bb0d3174a728ff8f9dbb51653b17e1e.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# Prevent GoogleTest from overriding your compiler flags
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

add_executable(almost_tests
    almost_tests.cpp
)

target_link_libraries(almost_tests
    PRIVATE
    almost::almost
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(almost_tests)
