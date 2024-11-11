![alt text](image.png)

# OpenGL Particle Simulation

A high-performance particle simulation system leveraging modern OpenGL features and GPU acceleration. This project demonstrates graphics programming techniques and parallel computing concepts through an interactive 2D particle system.

## Features

### Core Systems
- **GPU-Accelerated Particle Physics**
  - Real-time particle position and velocity updates
  - Gravity point influence system
  - Delta time-based simulation stepping

- **Modern OpenGL Pipeline**
  - Vertex/Fragment shaders for rendering
  - VAO/VBO management for efficient rendering
  - Custom shader compilation and management system

### Visualization & Interaction
- **Dynamic Grid System**
  - Configurable reference grid
  - Scale and spacing controls
  
- **Interactive Controls**
  - Mouse-based gravity point control
  - Camera pan and zoom functionality
  - Real-time simulation parameter adjustment

- **Performance Monitoring**
  - Real-time FPS counter
  - Frame time tracking
  - Particle count display
  - Delta time monitoring

### Dependencies
- **GLFW** - Window management and OpenGL context
- **CGLM** - Graphics mathematics library
- **GLAD** - OpenGL function loader
- **Dear ImGui** - Debug UI and statistics display
