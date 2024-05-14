# Smoothed Particle Hydrodynamics
Basic implementation of 3D SPH in C++ and OpenGL.
<!-- image -->
<p align="center">
  <img src="assets/images/demo.png" width="600">
</p>

## Features
- [x] OpenGL rendering
- [X] Boundary handling
- [x] Viscosity
- [ ] Spatial hashed grid
- [ ] Compute shader implementation
- [ ] Surface tension
- [ ] Interactive mode
- [ ] Screen space fluid rendering : [GDC 2010 Presentation](https://developer.download.nvidia.com/presentations/2010/gdc/Direct3D_Effects.pdf)


## Dependencies
- [GLFW](https://www.glfw.org/)
- [GLAD](https://glad.dav1d.de/)
- [GLM](https://github.com/g-truc/glm)
- [STB](https://github.com/nothings/stb)
- [Dear ImGui](https://github.com/ocornut/imgui)
- [tinyobjloader](https://github.com/tinyobjloader/tinyobjloader)

## Build and Run
Compiled using [mingw64](https://sourceforge.net/projects/mingw-w64/) and [CMake](https://cmake.org/).
```bash
# Clone the repository
git clone

# Build the project
cd SPH-Fluid-Simulations
mkdir build
cd build
cmake ..
make

# Run the executable
./SPH-Fluid-Simulations
```