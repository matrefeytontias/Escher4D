cmake_minimum_required(VERSION 3.13)

# Executable setup

project(Escher4D LANGUAGES C CXX)

set(SOURCES
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

# Resource generation

set(MODELS
    res/models/cube
    res/models/holedCube
    res/models/socket
    res/models/sphere)

foreach(MODEL ${MODELS})
    set(IN ${CMAKE_CURRENT_SOURCE_DIR}/${MODEL})
    set(OUT ${IN}.ele ${IN}.face)
    message(STATUS ${IN}.off)
    add_custom_command(
        OUTPUT ${OUT}
        DEPENDS ${MODEL}.off
        COMMAND $<TARGET_FILE:tetgen> -pYBNIQ ${IN}.off
    )
    list(APPEND MODELS_OUT ${OUT})
endforeach()

add_custom_target(
    GenerateResources ALL
    DEPENDS ${MODELS_OUT}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/res ${PROJECT_BINARY_DIR}
)

# Dependency setup

add_dependencies(EscherDemo GenerateResources)

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