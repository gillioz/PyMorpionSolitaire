cmake_minimum_required(VERSION 3.25)
project(PyMorpionSolitaire)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------

# pybind11 is provided by scikit-build-core as a build-time Python package.
# When developing locally you can also install it with: pip install pybind11
find_package(pybind11 CONFIG REQUIRED)

# jsoncpp is fetched automatically so that anyone can build the package
# without needing a system/conda installation of jsoncpp.
include(FetchContent)
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
# Core game logic — a private static library, not installed
# ---------------------------------------------------------------------------

add_library(morpion_core STATIC ${GAME_SOURCES})
target_include_directories(morpion_core PUBLIC include)
target_link_libraries(morpion_core PUBLIC jsoncpp_static)

# ---------------------------------------------------------------------------
# Python extension module
# ---------------------------------------------------------------------------

pybind11_add_module(CppMorpionSolitaire src/PyMorpionSolitaire.cpp)
target_link_libraries(CppMorpionSolitaire PRIVATE morpion_core)

# scikit-build-core looks for install() rules to know what to put in the wheel.
# The destination "pymorpionsolitaire" matches the Python package folder name.
install(TARGETS CppMorpionSolitaire DESTINATION pymorpionsolitaire)

# ---------------------------------------------------------------------------
# Optional developer targets (only built when -DBUILD_DEV_TOOLS=ON)
# Not included in the Python wheel.
# ---------------------------------------------------------------------------

option(BUILD_DEV_TOOLS "Build C++ test executables and PlayNestedMC" OFF)

if(BUILD_DEV_TOOLS)
    add_executable(GameTests tests/GameTest.cpp)
    target_link_libraries(GameTests PRIVATE morpion_core)

    add_executable(GraphGameTests tests/GraphGameTest.cpp)
    target_link_libraries(GraphGameTests PRIVATE morpion_core)

    add_executable(PlayNestedMC src/PlayNestedMC.cpp)
    target_link_libraries(PlayNestedMC PRIVATE morpion_core)
endif()