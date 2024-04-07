
add_library(imgui_backends
    imgui_impl_vulkan.cpp
    imgui_impl_sdl2.cpp
)

target_include_directories(imgui_backends PUBLIC ${PROJECT_SOURCE_DIR}/includes/imgui "C:/VulkanSDK/1.3.268.0/Include")
target_link_libraries(imgui_backends PUBLIC ${SDL2_LIBRARIES} C:/VulkanSDK/1.3.268.0/Lib/vulkan-1.lib)