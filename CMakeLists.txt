cmake_minimum_required( VERSION 3.17.0 ) # Latest version of CMake when this file was created.

# Use solution folders to organize projects
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project( optional LANGUAGES CXX )

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# Enable testing.
include(CTest)

if( BUILD_TESTING )
    add_subdirectory( tests )
    # Set the startup project.
    set_directory_properties( PROPERTIES 
        VS_STARTUP_PROJECT signal_tests
    )
endif( BUILD_TESTING )


