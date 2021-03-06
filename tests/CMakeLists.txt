cmake_minimum_required( VERSION 3.16.2 ) # Latest version of CMake when this file was created.
project( signals_tests )

include(FetchContent)
include(GoogleTest)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.10.0
)
FetchContent_MakeAvailable(googletest)

set( HEADER_FILES
    ../optional.hpp
)

add_executable( tests optional_tests.cpp ${HEADER_FILES} )
target_link_libraries( tests gtest gtest_main )
target_include_directories( tests 
    PUBLIC ../
)

gtest_discover_tests( tests )