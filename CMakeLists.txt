cmake_minimum_required(VERSION 3.18)
project(DisasterSim LANGUAGES C CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CUDA_STANDARD 17)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(assimp REQUIRED)
# Включаем директории
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)   
add_library(stb STATIC libs/stbi/stb_image.cc)
target_include_directories(stb PUBLIC libs/stbi)

file(GLOB_RECURSE SOURCE_FILES 
    src/main.cc
    src/core/*.cc
    src/data/*.cc
    src/core/simulation.cu
    src/graphics/*.cc
    src/graphics/OpenGL/*.cc
    src/scene/*.cc
)
set_source_files_properties(src/core/simulation.cu PROPERTIES LANGUAGE CUDA)
add_executable(disaster_sim ${SOURCE_FILES})

set_target_properties(disaster_sim PROPERTIES
    CUDA_RUNTIME_LIBRARY Shared
    CUDA_ARCHITECTURES "native" 
)

add_library(glad STATIC libs/glad/src/glad.c)
target_include_directories(glad PUBLIC libs/glad/include)
target_link_libraries(disaster_sim
    ${CUDA_LIBRARIES}
    OpenGL::GL
    glfw
    glad
    assimp::assimp
    stb
)

target_include_directories(disaster_sim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    libs/glad/include/
    libs/stbi/
    ${CUDA_INCLUDE_DIRS}
)

target_compile_options(disaster_sim PRIVATE
    $<$<COMPILE_LANGUAGE:CUDA>:--expt-relaxed-constexpr>
    $<$<COMPILE_LANGUAGE:CUDA>:-arch=sm_60>
)