cmake_minimum_required(VERSION 3.15...3.31)

set(CMAKE_CXX_STANDARD 20)

project(VulkanTesting VERSION 1.0 LANGUAGES CXX)

add_executable(VulkanTesting main.cpp)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

target_link_libraries(VulkanTesting glfw Vulkan::Vulkan)

# Shader compilation target
find_program(GLSLC glslc REQUIRED)

set(SHADER_SRC_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SHADER_BIN_DIR ${CMAKE_BINARY_DIR}/shaders)

# List your shaders here
set(SHADERS 
    ${SHADER_SRC_DIR}/triangle_app.vert
    ${SHADER_SRC_DIR}/triangle_app.frag
)

# Create output directories
file(MAKE_DIRECTORY ${SHADER_BIN_DIR})

set(SPIRV_FILES "")
set(HEADER_FILES "")

foreach(SHADER ${SHADERS})
    get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
    get_filename_component(SHADER_TYPE ${SHADER} EXT)
    string(REPLACE "." "_" SHADER_FULL_NAME "${SHADER_NAME}${SHADER_TYPE}")
    set(SPIRV_FILE ${SHADER_BIN_DIR}/${SHADER_FULL_NAME}.spv)

    # Add command to compile GLSL to SPIR-V
    add_custom_command(
        OUTPUT ${SPIRV_FILE}
        COMMAND ${GLSLC} ${SHADER} -o ${SPIRV_FILE}
        DEPENDS ${SHADER}
        COMMENT "Compiling GLSL shader: ${SHADER}"
        VERBATIM
    )

    list(APPEND SPIRV_FILES ${SPIRV_FILE})
endforeach()

# Create a custom target that generates all shaders
add_custom_target(compile_shaders ALL DEPENDS ${SPIRV_FILES})

# Resource files target
set(RESOURCES_SOURCE_DIR ${CMAKE_SOURCE_DIR}/resources)
set(RESOURCES_DEST_DIR ${CMAKE_BINARY_DIR}/resources)
file(GLOB RESOURCE_FILES ${RESOURCES_SOURCE_DIR})

add_custom_command(
    OUTPUT ${RESOURCES_DEST_DIR}/timestamp
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${RESOURCES_SOURCE_DIR} ${RESOURCES_DEST_DIR}
    COMMAND ${CMAKE_COMMAND} -E touch ${RESOURCES_DEST_DIR}/timestamp
    DEPENDS ${RESOURCE_FILES}
    COMMENT "Copying textures..."
)

add_custom_target(copy_textures ALL DEPENDS ${RESOURCES_DEST_DIR}/timestamp)