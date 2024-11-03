![alt text](image.png)

# OpenGL Particle Simulation

A real-time particle simulation system built with OpenGL, featuring GPU-accelerated compute shaders and SIMD-optimized CPU operations. The project demonstrates modern graphics programming techniques and high-performance computing concepts.

## Features

- **GPU-Accelerated Particle System**: Utilizes compute shaders for efficient particle physics calculations
- **SIMD Optimization**: CPU-side particle initialization using SSE/SSE2 instructions for improved performance
- **Modern OpenGL**: Leverages modern OpenGL features including:
  - Shader Storage Buffer Objects (SSBOs)
  - Compute shaders
  - Vertex Array Objects (VAO)
- **Dynamic Grid Rendering**: Background grid system for visual reference
- **Interactive**: Mouse-based particle interaction
- **Camera Controls**: 3D camera system for scene navigation

## Technical Details

- **Particle Management**: 
  - Efficient particle position and velocity management using OpenGL buffer objects
  - SIMD-optimized initialization routines
  - Dynamic velocity magnitude calculations
- **Shader Pipeline**:
  - Compute shader for particle physics
  - Vertex and fragment shaders for particle rendering
  - Custom shader loading and compilation system

## Dependencies 

- **GLFW**: Window management and OpenGL context creation (submodule)
- **cglm**: Mathematics library for matrix and vector operations (submodule)
- **GLAD**: OpenGL function loader 

## Future Improvements

- [ ] Add ImGui integration for real-time parameter tuning
- [ ] Implement additional particle effects and behaviors
- [ ] Add more advanced rendering techniques
- [ ] Support for particle collisions

## License

This project is open source and available under the MIT License.