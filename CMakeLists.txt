set(CMAKE_WARN_DEPRECATED OFF)
cmake_minimum_required(VERSION 3.9)
project(SPH-Fluid-Simulation LANGUAGES CXX CUDA)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED True)
# Enable GPU-specific optimizations
set(CMAKE_CUDA_ARCHITECTURES "80") # Adjust based on your GPU's compute capability
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O1 -Wall")
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -O1 -Xcompiler=-Wall")
if(UNIX)
    set(OpenGL_GL_PREFERENCE "GLVND")
endif()

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)


# Add glm
FetchContent_Declare(
    glm
    GIT_REPOSITORY "https://github.com/g-truc/glm.git"
    GIT_TAG "master"
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(glm)
include_directories(${glm_SOURCE_DIR})

# Add Dear ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY "https://github.com/ocornut/imgui.git"
    GIT_TAG "master"
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(imgui)
include_directories(${imgui_SOURCE_DIR})
include_directories(${imgui_SOURCE_DIR}/backends)
file(GLOB IMGUI_SOURCES ${imgui_SOURCE_DIR}/*.cpp ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)

# Add GLAD
FetchContent_Declare(
    glad
    GIT_REPOSITORY "https://github.com/Dav1dde/glad.git"
    GIT_TAG "master"
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(glad)

# Add glfw
FetchContent_Declare(
    glfw
    GIT_REPOSITORY "https://github.com/glfw/glfw.git"
    GIT_TAG "master"
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(glfw)

# Find OpenGL
find_package(OpenGL REQUIRED)
if(NOT OPENGL_FOUND)
    message(ERROR "OpenGL not found")
endif()

# Find OMP
find_package(OpenMP REQUIRED)
if(NOT OpenMP_CXX_FOUND)
    message(ERROR "OpenMP not found")
endif()

# Add source files
file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")

# Add CUDA source files
file(GLOB_RECURSE CUDA_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/src/*.cu")

# Add header files
file(GLOB_RECURSE PROJECT_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/include/*.hpp")

# Add cuda header files
file(GLOB_RECURSE CUDA_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/include/*.cuh")

# Add executable
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES} ${CUDA_SOURCES} ${PROJECT_HEADERS} ${CUDA_HEADERS} ${IMGUI_SOURCES})

# Set CUDA properties for the target
set_target_properties(${PROJECT_NAME} PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON # Enable separable compilation for CUDA
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE glad glfw ${CMAKE_DL_LIBS} OpenGL::GL OpenMP::OpenMP_CXX)

# External include directories
target_include_directories(${PROJECT_NAME} PRIVATE ext/stb_image ext/tinyobjloader include)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
target_compile_definitions(${PROJECT_NAME} PUBLIC ASSETS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/") # Set the asset path macro to the absolute path on the dev machine