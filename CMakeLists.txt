cmake_minimum_required(VERSION 3.0.0)

project(PathTracer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SDL2_DIR "${PROJECT_SOURCE_DIR}/includes/SDL2/cmake")
set(SDL2_image_DIR "${PROJECT_SOURCE_DIR}/includes/SDL2/cmake")
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)

add_executable(PathTracer PathTracer.cpp)

target_include_directories(PathTracer PUBLIC ${SDL2_INCLUDE_DIRS})
target_include_directories(PathTracer PUBLIC "${PROJECT_SOURCE_DIR}/includes/glad/include")
target_include_directories(PathTracer PUBLIC "${PROJECT_SOURCE_DIR}/includes/imgui")
target_include_directories(PathTracer PUBLIC "${PROJECT_SOURCE_DIR}/includes/imgui/backends")
target_include_directories(PathTracer PUBLIC "${PROJECT_SOURCE_DIR}/includes")
add_subdirectory(${PROJECT_SOURCE_DIR}/includes/glad)
add_subdirectory(${PROJECT_SOURCE_DIR}/includes/imgui)
add_subdirectory(${PROJECT_SOURCE_DIR}/includes/imgui/backends)
target_link_libraries(PathTracer ${SDL2_LIBRARIES} SDL2_image::SDL2_image glad imgui imgui_backends)

# Thank You So Much https://stackoverflow.com/questions/10671916/how-to-copy-dll-files-into-the-same-folder-as-the-executable-using-cmake/10672739#10672739
# Check If It Is x86 Or x64
if(CMAKE_SIZEOF_VOID_P STREQUAL "4")
    add_custom_command(TARGET PathTracer POST_BUILD COMMAND ${CMAKE_COMMAND} 
    -E copy_if_different "${PROJECT_SOURCE_DIR}/includes/SDL2/lib/x86/SDL2.dll" $<TARGET_FILE_DIR:PathTracer>)
    add_custom_command(TARGET PathTracer POST_BUILD COMMAND ${CMAKE_COMMAND} 
    -E copy_if_different "${PROJECT_SOURCE_DIR}/includes/SDL2/lib/x86/SDL2_image.dll" $<TARGET_FILE_DIR:PathTracer>)
elseif(CMAKE_SIZEOF_VOID_P STREQUAL "8")
    add_custom_command(TARGET PathTracer POST_BUILD COMMAND ${CMAKE_COMMAND} 
    -E copy_if_different "${PROJECT_SOURCE_DIR}/includes/SDL2/lib/x64/SDL2.dll" $<TARGET_FILE_DIR:PathTracer>)
    add_custom_command(TARGET PathTracer POST_BUILD COMMAND ${CMAKE_COMMAND} 
    -E copy_if_different "${PROJECT_SOURCE_DIR}/includes/SDL2/lib/x64/SDL2_image.dll" $<TARGET_FILE_DIR:PathTracer>)
endif()

add_custom_command(TARGET PathTracer POST_BUILD COMMAND ${CMAKE_COMMAND} 
-E copy_directory "${PROJECT_SOURCE_DIR}/Shaders" $<TARGET_FILE_DIR:PathTracer>)
