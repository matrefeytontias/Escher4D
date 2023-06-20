cmake_minimum_required(VERSION 3.13)

### Escher4D

# Library setup

add_library(Escher)
target_include_directories(Escher PUBLIC .)

set(PUBLIC_SOURCES
    # Top level
    Escher4D/Camera4.hpp
    Escher4D/Context.h
    Escher4D/FSQuadRenderContext.hpp
    Escher4D/HierarchicalBuffer.hpp
    Escher4D/MathUtil.hpp
    Escher4D/Model4RenderContext.hpp
    Escher4D/Object4.hpp
    Escher4D/RenderContext.hpp
    # Escher4D/Rotor4.hpp
    Escher4D/ShadowHypervolumes.hpp
    Escher4D/Transform4.hpp
    Escher4D/utils.hpp
    # Meshes
    Escher4D/meshes/Geometry4.hpp
    Escher4D/meshes/mesh_loading.hpp
)
set(PRIVATE_SOURCES
    # Top level
    Escher4D/MathUtil.cpp
    Escher4D/Model4RenderContext.cpp
    Escher4D/utils.cpp
    # Meshes
    Escher4D/meshes/mesh_loading.cpp
)

target_sources(Escher PUBLIC ${PUBLIC_SOURCES})
target_sources(Escher PRIVATE ${PRIVATE_SOURCES})

target_compile_features(Escher PRIVATE cxx_std_17)

# Link third-party libraries

target_link_libraries(Escher PUBLIC glfw imgui imgui-glfw imgui-opengl3 Empty)
