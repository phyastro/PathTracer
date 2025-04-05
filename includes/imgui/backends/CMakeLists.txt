add_library(imgui_backends
    imgui_impl_vulkan.cpp
    imgui_impl_glfw.cpp
)

target_include_directories(imgui_backends PUBLIC ${PROJECT_SOURCE_DIR}/includes)
target_include_directories(imgui_backends PUBLIC ${PROJECT_SOURCE_DIR}/includes/imgui)

add_library(vulkan SHARED IMPORTED)
set_property(TARGET vulkan PROPERTY IMPORTED_IMPLIB ${CMAKE_BINARY_DIR}/libvulkan-1.dll.a)
set_property(TARGET vulkan PROPERTY IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/vulkan-1.dll)

target_link_libraries(imgui_backends PUBLIC vulkan)
