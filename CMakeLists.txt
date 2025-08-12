cmake_minimum_required(VERSION 3.10)
project(SimpleFPS)

set(CMAKE_CXX_STANDARD 20)

find_package(OpenGL REQUIRED)

# Adjust these to your system paths
include_directories(
    C:/src/third_party/glfw/include
    C:/src/third_party/glew/include
    C:/src/third_party/glm
)

link_directories(
    C:/src/third_party/glfw/lib-vc2019 
    C:/src/third_party/glew/lib/Release/x64
)

add_executable(SimpleFPS main.cpp)
target_link_libraries(SimpleFPS PRIVATE glfw3 glew32s opengl32)


add_executable(test test.cpp)
target_link_libraries(test PRIVATE glfw3 glew32s opengl32)
# target_compile_definitions(test PRIVATE GLEW_STATIC)
# target_include_directories(test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} C:/src/third_party/glfw/include
#     C:/src/third_party/glew/include
#     C:/src/third_party/glm)


add_custom_command(
    TARGET SimpleFPS POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:SimpleFPS>/assets
)