
add_library(glad
    src/glad.c
)

target_include_directories(glad PUBLIC ${PROJECT_SOURCE_DIR}/includes/glad/include)