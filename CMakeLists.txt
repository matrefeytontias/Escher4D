cmake_minimum_required(VERSION 3.13)

### Project setup

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(Escher4D DESCRIPTION "Experimental slicing-based 4D graphics engine" LANGUAGES C CXX)

# Main library

add_subdirectory(Escher)

# Examples subfolder

option(ESCHER_BUILD_EXAMPLES "Build the Escher4D example programs" False)
if(ESCHER_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

### Third-party libraries

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_USE_HYBRID_HPG ON CACHE BOOL "" FORCE)
add_subdirectory(ThirdParty/glfw)
set_target_properties(glfw PROPERTIES FOLDER "ThirdParty/GLFW3")
set_target_properties(update_mappings PROPERTIES FOLDER "ThirdParty/GLFW3")

# imgui
# ThirdParty/imgui/CMakeLists.txt is configured with GLFW and OpenGL3
add_subdirectory(ThirdParty/imgui)
set_target_properties(imgui PROPERTIES FOLDER "ThirdParty")

# Empty
set(EMPTY_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
add_subdirectory(ThirdParty/Empty)
set_target_properties(Empty PROPERTIES FOLDER "ThirdParty")

# tetgen
add_subdirectory(ThirdParty/tetgen1.6.0)
set_target_properties(tet PROPERTIES FOLDER "tetgen")
set_target_properties(tetgen PROPERTIES FOLDER "tetgen")
