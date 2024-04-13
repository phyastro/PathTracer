
add_library(imgui_backends
    imgui_impl_vulkan.cpp
    imgui_impl_glfw.cpp
)

target_include_directories(imgui_backends PUBLIC ${PROJECT_SOURCE_DIR}/includes/imgui)
target_link_libraries(imgui_backends PUBLIC glfw Vulkan::Vulkan)