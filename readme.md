![alt text](image.png)

# OpenGL Particle Simulation

A high-performance particle simulation system leveraging modern OpenGL features and GPU acceleration. This project demonstrates graphics programming techniques and parallel computing concepts through an interactive 3D particle system.

## Features

### Core Systems
- **GPU-Accelerated Particle Physics**
  - Compute shader-based particle calculations
  - SSBO (Shader Storage Buffer Object) management
  - Real-time position and velocity updates
  
- **SIMD Optimizations**
  - CPU-side particle initialization using SSE/SSE2
  - Optimized memory layout for vectorized operations

- **Modern OpenGL Pipeline**
  - Compute shaders for physics calculations
  - Vertex/Fragment shaders for rendering
  - VAO/VBO management for efficient rendering
  - Custom shader compilation and management system

### Visualization & Interaction
- **Dynamic Grid System**
  - Grid for spatial reference
  
- **Interactive Controls**
  - Mouse-based particle interaction
  - 2D camera navigation system

### Dependencies
- **GLFW** - Window management and OpenGL context
- **CGLM** - Optimized graphics mathematics library
- **GLAD** - OpenGL function loader
- **Dear ImGui** - User interface (dummy init done)

## Future Development

- [ ] ImGui integration for real-time parameter tuning
- [ ] Additional particle behaviors and effects
- [ ] Advanced rendering techniques (shadows, lighting)
- [ ] Particle collision system
- [ ] Multi-threaded CPU particle updates
- [ ] Particle emission patterns and systems

## License

 GNU General Public License
