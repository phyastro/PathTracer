cmake_minimum_required(VERSION 3.0.0)

project(PathTracer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(PathTracer src/pathtracer.cpp)

target_include_directories(PathTracer PUBLIC "${PROJECT_SOURCE_DIR}/includes")
target_include_directories(PathTracer PUBLIC "C:/VulkanSDK/1.3.268.0/Include")
target_include_directories(PathTracer PUBLIC "${PROJECT_SOURCE_DIR}/includes/imgui")
target_include_directories(PathTracer PUBLIC "${PROJECT_SOURCE_DIR}/includes/imgui/backends")
add_subdirectory(${PROJECT_SOURCE_DIR}/includes/glfw)
add_subdirectory(${PROJECT_SOURCE_DIR}/includes/imgui)
add_subdirectory(${PROJECT_SOURCE_DIR}/includes/imgui/backends)
target_link_libraries(PathTracer glfw C:/VulkanSDK/1.3.268.0/Lib/vulkan-1.lib imgui imgui_backends)
