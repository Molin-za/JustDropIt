cmake_minimum_required(VERSION 3.15)
project(JustDropIt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
)
FetchContent_MakeAvailable(raylib)

add_executable(JustDropIt
    src/main.cpp
    src/Block.cpp
    src/Common.h
    src/GridManager.cpp
    src/GridManager.h
    src/MiniFb.cpp
    src/MiniFb.h
    src/OrderManager.cpp
    src/OrderManager.h
    src/ShopManager.cpp
    src/ShopManager.h
    src/Block.h
)

target_include_directories(JustDropIt PRIVATE src)
target_link_libraries(JustDropIt PRIVATE raylib)
target_compile_options(JustDropIt PRIVATE /utf-8)
