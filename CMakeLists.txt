cmake_minimum_required(VERSION 3.31)

# Our C++ project ...
project(gf2 DESCRIPTION "C++ classes for working in GF(2)" LANGUAGES CXX)

# Add a target for the "library" we are building (${PROJECT_NAME} is header only -- hence INTERFACE).
# Also add an alias that prepends a "namespace" -- if clients use that to link to, they get better error messages.
add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# We use C++23 features.
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_23)

# Where to find the headers (how to resolve `#include <gf2/gf2.h>`).
target_sources(${PROJECT_NAME} INTERFACE
    FILE_SET    library_headers
    TYPE        HEADERS
    BASE_DIRS   include/)

# Set the GF2_BUILD_MODULES flag to enable building the library as a C++ module.
# This is off by default since not all compilers support C++ modules yet.
option(GF2_BUILD_MODULES "Build gf2 as a C++ module" OFF)
if (GF2_BUILD_MODULES)
    add_subdirectory(modules)
endif()

# That's it unless we are developing the library instead of just using it ...
if (PROJECT_IS_TOP_LEVEL)

    # Append our local directory of CMake modules to the default ones searched by CMake.
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

    # Prevent in-source builds for the example programs.
    include(disable_in_source_builds)

    # Make the compiler issue warnings for "bad" code, etc.
    include(compiler_init)
    compiler_init(${PROJECT_NAME})

    # Debug builds have the DEBUG flag set.
    target_compile_definitions(${PROJECT_NAME} INTERFACE $<$<CONFIG:Debug>: -DDEBUG>)

    # Release builds have the NDEBUG flag set.
    target_compile_definitions(${PROJECT_NAME} INTERFACE $<$<CONFIG:Release>: -DNDEBUG>)

    # For neatness, we put all example/test executables in build/bin/.
    include(GNUInstallDirs)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}")

    # -------------------------------------------------------------------------------------------------------------------
    # Add targets for the programs in the `examples/` directory (each .cpp file becomes a target).
    # -------------------------------------------------------------------------------------------------------------------

    # Our example programs use the header-only `utilities` library -- grab the current version from GitHub.
    include(fetch_content)
    fetch_content(utilities URL https://github.com/nessan/utilities/releases/download/current/utilities.zip)

    # Walk through the `examples/` directory and build a target for each .cpp file with appropriate linkage.
    # We have a CMake module that makes that traversal straightforward.
    include(add_executables)
    add_executables(${CMAKE_SOURCE_DIR}/examples
        COMBINED_TARGET "all_examples"
        LIBRARIES ${PROJECT_NAME}::${PROJECT_NAME} utilities::utilities)

    # -------------------------------------------------------------------------------------------------------------------
    # Targets for our embedded tests programs.
    # -------------------------------------------------------------------------------------------------------------------

    # Extract the tests from the comments in the header files and create a target for each corresponding test program.
    # We have a CMake module and a Python script that make that straightforward.
    include(doxytest)
    doxytest(${CMAKE_SOURCE_DIR}/include/gf2
        INCLUDES "<gf2/namespace.h>" LIBRARIES ${PROJECT_NAME}::${PROJECT_NAME})

endif()
