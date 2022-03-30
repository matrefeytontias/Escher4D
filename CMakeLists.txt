cmake_minimum_required(VERSION 3.13)

# Executable setup

project(Escher4D LANGUAGES C CXX)

set(SOURCES ${SOURCES}
    include/Camera4.hpp
    include/Context.h
    include/FSQuadRenderContext.hpp
    include/Geometry4.hpp
    include/HierarchicalBuffer.hpp
    include/MathUtil.hpp
    include/Model4RenderContext.hpp
    include/Object4.hpp
    include/OFFLoader.hpp
    include/RenderContext.hpp
    # include/Rotor4.hpp
    include/ShadowHypervolumes.hpp
    include/Transform4.hpp
    include/utils.hpp
    include/glad/glad.h
    include/KHR/khrplatform.h
    src/demos.cpp
    src/glad.c
    src/main.cpp
    src/MathUtil.cpp
    src/Model4RenderContext.cpp
    src/utils.cpp)

add_executable(EscherDemo ${SOURCES})

target_compile_features(EscherDemo PRIVATE cxx_std_17)

target_include_directories(EscherDemo PUBLIC include)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT EscherDemo)

add_custom_target(CopyResources ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/res ${CMAKE_BINARY_DIR}
    DEPENDS EscherDemo)

### Third-party libraries

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_USE_HYBRID_HPG ON CACHE BOOL "" FORCE)
add_subdirectory(ThirdParty/glfw)

# imgui
# ThirdParty/imgui/CMakeLists.txt is configured with GLFW and OpenGL3
add_subdirectory(ThirdParty/imgui)

# Empty
set(EMPTY_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
add_subdirectory(ThirdParty/Empty)

# tetgen
add_subdirectory(ThirdParty/tetgen1.6.0)

# Link everything
target_link_libraries(EscherDemo PUBLIC glfw imgui imgui-glfw imgui-opengl3 Empty)