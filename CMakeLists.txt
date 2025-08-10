cmake_minimum_required(VERSION 3.10)
project(SimpleFPS)

set(CMAKE_CXX_STANDARD 11)

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

target_link_libraries(SimpleFPS
    OpenGL::GL
    glfw3
    glew32s
    opengl32
    glu32
)
