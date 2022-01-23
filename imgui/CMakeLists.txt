cmake_minimum_required(VERSION 3.2)
project(imgui CXX)

add_library(imgui
        imgui.cpp
        imgui_demo.cpp
        imgui_draw.cpp
        imgui_tables.cpp
        imgui_widgets.cpp
        imgui_impl_sdl.cpp
        imgui_impl_dx11.cpp)

target_include_directories(imgui PUBLIC include)

target_link_libraries(imgui SDL2-static)