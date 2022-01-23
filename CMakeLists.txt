cmake_minimum_required(VERSION 3.21)
project(playground CXX)

set(BUILD_SHARED_LIBS OFF)

set(SDL_LIBC ON CACHE BOOL "" FORCE)
add_subdirectory(SDL EXCLUDE_FROM_ALL)

add_subdirectory(imgui EXCLUDE_FROM_ALL)

set(CMAKE_CXX_STANDARD 17)

add_executable(playground main.cpp app.cpp app.h)

target_link_libraries(playground SDL2-static SDL2main D3D11 imgui)

target_compile_definitions(playground PRIVATE WIN32_LEAN_AND_MEAN)
