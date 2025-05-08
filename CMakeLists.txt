cmake_minimum_required(VERSION 4.0.1)


project(paint)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(IMGUI_BACKENDS 
  imgui/imgui.cpp 
  imgui/imgui_demo.cpp
  imgui/imgui_draw.cpp
  imgui/imgui_tables.cpp
  imgui/imgui_widgets.cpp 
  imgui/backends/imgui_impl_sdl2.cpp 
  imgui/backends/imgui_impl_sdlrenderer2.cpp
)

add_executable(paint src/main.cpp src/paint.cpp ${IMGUI_BACKENDS})

target_include_directories(paint PRIVATE imgui)

add_subdirectory(SDL)
target_link_libraries(paint SDL2::SDL2)
