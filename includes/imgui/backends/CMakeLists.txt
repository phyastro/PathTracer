
add_library(imgui_backends
    imgui_impl_opengl3.cpp
    imgui_impl_sdl2.cpp
)

target_include_directories(imgui_backends PUBLIC ${SDL2_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/includes/imgui)