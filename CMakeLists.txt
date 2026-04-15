cmake_minimum_required(VERSION 3.25)
project(PyMorpionSolitaire)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ensures all targets (including static libs linked into the Python extension)
# are compiled with -fPIC, which is required when linking into a shared object.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Default to Release (-O3 -DNDEBUG) for manual cmake builds.
# scikit-build-core already sets this to Release when building the package.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------

include(FetchContent)

# pybind11 is provided automatically by scikit-build-core during pip install.
# For direct cmake invocations (e.g. cmake -DBUILD_DEV_TOOLS=ON) it is
# fetched automatically if not already present on the system.
find_package(pybind11 CONFIG QUIET)
if(NOT pybind11_FOUND)
    FetchContent_Declare(
        pybind11
        GIT_REPOSITORY https://github.com/pybind/pybind11.git
        GIT_TAG        v2.13.6
    )
    FetchContent_MakeAvailable(pybind11)
endif()

# jsoncpp is fetched automatically so that anyone can build the package
# without needing a system/conda installation of jsoncpp.
FetchContent_Declare(
    jsoncpp
    GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp.git
    GIT_TAG        1.9.5
)
# Build jsoncpp as a static library so it is baked into the Python extension
# and users don't need a separate shared library on their PATH/LD_LIBRARY_PATH.
set(JSONCPP_WITH_TESTS        OFF CACHE BOOL "" FORCE)
set(JSONCPP_WITH_POST_BUILD_UNITTEST OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS         OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC_LIBS         ON  CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(jsoncpp)

# ---------------------------------------------------------------------------
# Source files shared by the Python extension (and optionally the dev tools)
# ---------------------------------------------------------------------------

set(GAME_SOURCES
        src/Game.cpp
        src/Grid.cpp
        src/Image.cpp
        src/Line.cpp
        src/Coordinates.cpp
        src/Point.cpp
        src/GridFootprint.cpp
        src/GridDTO.cpp
        src/GraphGame.cpp)

# ---------------------------------------------------------------------------
# Build-time configuration
# ---------------------------------------------------------------------------

set(IMAGESIZE 192 CACHE STRING
    "Size of the square board image array (default: 192). Must be large \
enough to contain any grid position reachable in a game.")

# ---------------------------------------------------------------------------
# Core game logic — a private static library, not installed
# ---------------------------------------------------------------------------

add_library(morpion_core STATIC ${GAME_SOURCES})
target_include_directories(morpion_core PUBLIC include)
target_link_libraries(morpion_core PUBLIC jsoncpp_static)
target_compile_definitions(morpion_core PUBLIC IMAGESIZE=${IMAGESIZE})

# ---------------------------------------------------------------------------
# Python extension module
# ---------------------------------------------------------------------------

pybind11_add_module(CppMorpionSolitaire src/PyMorpionSolitaire.cpp)
target_link_libraries(CppMorpionSolitaire PRIVATE morpion_core)

# scikit-build-core looks for install() rules to know what to put in the wheel.
# The destination "PyMorpionSolitaire" matches the Python package folder name.
install(TARGETS CppMorpionSolitaire DESTINATION PyMorpionSolitaire)

# ---------------------------------------------------------------------------
# Optional developer targets (only built when -DBUILD_DEV_TOOLS=ON)
# Not included in the Python wheel.
# ---------------------------------------------------------------------------

option(BUILD_DEV_TOOLS "Build C++ test executables and PlayNestedMC" OFF)

if(BUILD_DEV_TOOLS)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.6.0
    )
    FetchContent_MakeAvailable(Catch2)

    # Place all dev executables directly in build/ for easy access
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

    add_executable(GameTests tests/GameTest.cpp)
    target_link_libraries(GameTests PRIVATE morpion_core Catch2::Catch2WithMain)

    add_executable(GraphGameTests tests/GraphGameTest.cpp)
    target_link_libraries(GraphGameTests PRIVATE morpion_core Catch2::Catch2WithMain)

    add_executable(PlayNestedMC src/PlayNestedMC.cpp)
    target_link_libraries(PlayNestedMC PRIVATE morpion_core)

    # Register tests with CTest
    include(CTest)
    include(Catch)
    catch_discover_tests(GameTests)
    catch_discover_tests(GraphGameTests PROPERTIES TIMEOUT 30)
endif()