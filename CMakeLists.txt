cmake_minimum_required(VERSION 3.14)
project(lilNES)

set(CMAKE_CXX_STANDARD 17)
include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG master
)
FetchContent_MakeAvailable(raylib)

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(lilNES ${SOURCES})
target_link_libraries(lilNES PRIVATE raylib)