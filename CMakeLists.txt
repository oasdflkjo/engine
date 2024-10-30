cmake_minimum_required(VERSION 3.15)
project(ParticleSimulation)

# Set C standard
set(CMAKE_C_STANDARD 11)

# Define source files
set(SOURCE_FILES src/main.c src/camera.c src/world.c)

# Add GLFW as a subdirectory (assuming GLFW is in external/glfw)
add_subdirectory(external/glfw)

# Add GLAD
add_library(glad external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

# Add cglm as a subdirectory
add_subdirectory(external/cglm)

# Add executable and link GLFW, GLAD, cglm, and OpenGL
add_executable(main ${SOURCE_FILES})
target_link_libraries(main glfw glad cglm opengl32)

# Set the output directory for the executable to the project root
set_target_properties(main PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

# Add after your existing include_directories() calls
include_directories(${CMAKE_SOURCE_DIR}/include)
