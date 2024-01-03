cmake_minimum_required(VERSION 3.0.0)

project(VulkanApp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SDL2_DIR "${PROJECT_SOURCE_DIR}/includes/SDL2/cmake")
set(SDL2_image_DIR "${PROJECT_SOURCE_DIR}/includes/SDL2/cmake")
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)

add_executable(VulkanApp src/vulkanapp.cpp)

target_include_directories(VulkanApp PUBLIC ${SDL2_INCLUDE_DIRS})
target_include_directories(VulkanApp PUBLIC "${PROJECT_SOURCE_DIR}/includes")
target_include_directories(VulkanApp PUBLIC "C:/VulkanSDK/1.3.268.0/Include")
target_link_libraries(VulkanApp ${SDL2_LIBRARIES} SDL2_image::SDL2_image C:/VulkanSDK/1.3.268.0/Lib/vulkan-1.lib)

# Thank You So Much https://stackoverflow.com/questions/10671916/how-to-copy-dll-files-into-the-same-folder-as-the-executable-using-cmake/10672739#10672739
# Check If It Is x86 Or x64
if(CMAKE_SIZEOF_VOID_P STREQUAL "4")
    add_custom_command(TARGET VulkanApp POST_BUILD COMMAND ${CMAKE_COMMAND} 
    -E copy_if_different "${PROJECT_SOURCE_DIR}/includes/SDL2/lib/x86/SDL2.dll" $<TARGET_FILE_DIR:VulkanApp>)
    add_custom_command(TARGET VulkanApp POST_BUILD COMMAND ${CMAKE_COMMAND} 
    -E copy_if_different "${PROJECT_SOURCE_DIR}/includes/SDL2/lib/x86/SDL2_image.dll" $<TARGET_FILE_DIR:VulkanApp>)
elseif(CMAKE_SIZEOF_VOID_P STREQUAL "8")
    add_custom_command(TARGET VulkanApp POST_BUILD COMMAND ${CMAKE_COMMAND} 
    -E copy_if_different "${PROJECT_SOURCE_DIR}/includes/SDL2/lib/x64/SDL2.dll" $<TARGET_FILE_DIR:VulkanApp>)
    add_custom_command(TARGET VulkanApp POST_BUILD COMMAND ${CMAKE_COMMAND} 
    -E copy_if_different "${PROJECT_SOURCE_DIR}/includes/SDL2/lib/x64/SDL2_image.dll" $<TARGET_FILE_DIR:VulkanApp>)
endif()
