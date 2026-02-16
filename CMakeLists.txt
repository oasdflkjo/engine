cmake_minimum_required(VERSION 3.15)
project(ParticleSimulation)
option(ENABLE_RECORDING "Enable in-engine FFmpeg VAAPI recording" OFF)

# Enable both C and C++
enable_language(C CXX)

# Set language standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)

# Find all source files using wildcards
file(GLOB_RECURSE SOURCES 
    "src/*.c"
    "src/*.cpp"
)

# Find all header files (optional, but good for IDE support)
file(GLOB_RECURSE HEADERS
    "include/*.h"
    "include/*.hpp"
)

# Add GLFW
add_subdirectory(external/glfw)

# Add GLAD
add_library(glad external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

# Add cglm
add_subdirectory(external/cglm)

# Create ImGui library
add_library(imgui STATIC
    ${CMAKE_SOURCE_DIR}/external/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui/imgui_demo.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui/imgui_tables.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui/backends/imgui_impl_glfw.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${CMAKE_SOURCE_DIR}/external/imgui
    ${CMAKE_SOURCE_DIR}/external/imgui/backends
    ${CMAKE_SOURCE_DIR}/external/glfw/include
)

# Link ImGui with GLFW
target_link_libraries(imgui PUBLIC glfw)

# Add main executable
add_executable(main ${SOURCES} ${HEADERS})

# Set include directories
target_include_directories(main PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/external/imgui
    ${CMAKE_SOURCE_DIR}/external/imgui/backends
    ${CMAKE_SOURCE_DIR}/external/glfw/include
    ${CMAKE_SOURCE_DIR}/external/glad/include
    ${CMAKE_SOURCE_DIR}/external/cglm/include
)

# Link all libraries in one command
target_link_libraries(main PRIVATE
    glfw
    glad
    cglm
    imgui
    OpenGL::GL
)

if(ENABLE_RECORDING)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
        libavcodec
        libavformat
        libavutil
        libswscale
    )
    target_compile_definitions(main PRIVATE ENABLE_RECORDING=1)
    target_link_libraries(main PRIVATE PkgConfig::FFMPEG)
endif()

# Set the output directory for the executable
set_target_properties(main PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)
