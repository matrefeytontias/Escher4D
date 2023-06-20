cmake_minimum_required(VERSION 3.13)

add_executable(EightRoomsDemo main.cpp demos.cpp)

set_target_properties(EightRoomsDemo PROPERTIES FOLDER "Examples")
target_compile_features(EightRoomsDemo PRIVATE cxx_std_17)

# Resource generation

set(MODELS
    res/models/cube
    res/models/holedCube
    res/models/socket
    res/models/sphere)

foreach(MODEL ${MODELS})
    set(IN ${CMAKE_CURRENT_SOURCE_DIR}/${MODEL})
    set(OUT ${IN}.ele ${IN}.face)
    add_custom_command(
        OUTPUT ${OUT}
        DEPENDS ${MODEL}.off
        COMMAND $<TARGET_FILE:tetgen> -pYBNIQ ${IN}.off
    )
    list(APPEND MODELS_OUT ${OUT})
endforeach()

add_custom_target(
    CookResources ALL
    DEPENDS ${MODELS_OUT}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/res ${CMAKE_CURRENT_BINARY_DIR}
)

add_dependencies(EightRoomsDemo CookResources)

# Dependencies

target_link_libraries(EightRoomsDemo PUBLIC Escher)
