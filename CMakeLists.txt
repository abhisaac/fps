cmake_minimum_required(VERSION 3.10)
project(SimpleFPS)

set(CMAKE_CXX_STANDARD 20)

find_package(OpenGL REQUIRED)

if(WIN32)
    set(LIBS opengl32 glew32s glfw3)
   
elseif(APPLE)
    set(LIBS glew glfw "-framework OpenGL")
else()
    set(LIBS glew glfw GL)
    
endif()

# Adjust these to your system paths
if (WIN32)
include_directories(
    C:/src/third_party/glfw/include
    C:/src/third_party/glew/include
    C:/src/third_party/glm
)

link_directories(
    C:/src/third_party/glfw/lib-vc2019 
    C:/src/third_party/glew/lib/Release/x64
)
elseif(APPLE)
 find_package(PkgConfig REQUIRED)
    pkg_search_module(GLFW REQUIRED glfw)
    pkg_search_module(GLEW REQUIRED glew)
    pkg_search_module(GLM REQUIRED glm)
    include_directories(
        ${GLFW_INCLUDE_DIRS}
        ${GLEW_INCLUDE_DIRS}
        ${GLM_INCLUDE_DIRS}
    )
    link_directories(
        ${GLFW_LIBRARY_DIRS}
        ${GLEW_LIBRARY_DIRS}
    )
endif()


set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/imgui)
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC ${IMGUI_DIR})


add_executable(SimpleFPS main.cpp)
target_link_libraries(SimpleFPS PRIVATE ${LIBS} imgui)
target_include_directories(SimpleFPS PUBLIC ${IMGUI_DIR})

add_executable(test test.cpp)
target_link_libraries(test PRIVATE ${LIBS})
# target_compile_definitions(test PRIVATE GLEW_STATIC)


add_custom_command(
    TARGET SimpleFPS POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:SimpleFPS>/assets
)