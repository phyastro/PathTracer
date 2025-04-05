cmake_minimum_required(VERSION 3.0.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(PathTracer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(PathTracer src/pathtracer.cpp)

target_include_directories(PathTracer PUBLIC "${PROJECT_SOURCE_DIR}/includes")
target_include_directories(PathTracer PUBLIC "${PROJECT_SOURCE_DIR}/includes/glfw")
target_include_directories(PathTracer PUBLIC "${PROJECT_SOURCE_DIR}/includes/imgui")
target_include_directories(PathTracer PUBLIC "${PROJECT_SOURCE_DIR}/includes/imgui/backends")
target_include_directories(PathTracer PUBLIC "${PROJECT_SOURCE_DIR}/includes/glslang")

add_subdirectory(${PROJECT_SOURCE_DIR}/includes/imgui)
add_subdirectory(${PROJECT_SOURCE_DIR}/includes/imgui/backends)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

add_library(vulkan SHARED IMPORTED)
set_property(TARGET vulkan PROPERTY IMPORTED_IMPLIB ${CMAKE_BINARY_DIR}/libvulkan-1.dll.a)
set_property(TARGET vulkan PROPERTY IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/vulkan-1.dll)

add_library(glfw SHARED IMPORTED)
set_property(TARGET glfw PROPERTY IMPORTED_IMPLIB ${CMAKE_BINARY_DIR}/libglfw3dll.a)
set_property(TARGET glfw PROPERTY IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/glfw3.dll)

add_library(glslang SHARED IMPORTED)
set_property(TARGET glslang PROPERTY IMPORTED_IMPLIB ${CMAKE_BINARY_DIR}/libglslang.dll.a)
set_property(TARGET glslang PROPERTY IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/glslang.dll)

add_library(glslang-default-resource-limits SHARED IMPORTED)
set_property(TARGET glslang-default-resource-limits PROPERTY IMPORTED_IMPLIB ${CMAKE_BINARY_DIR}/libglslang-default-resource-limits.dll.a)
set_property(TARGET glslang-default-resource-limits PROPERTY IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/libglslang-default-resource-limits.dll)

target_link_libraries(PathTracer PUBLIC vulkan glfw imgui imgui_backends glfw glslang glslang-default-resource-limits)
