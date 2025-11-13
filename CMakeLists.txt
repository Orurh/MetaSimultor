cmake_minimum_required(VERSION 3.24)
project(DisasterSim LANGUAGES C CXX CUDA)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CUDA_STANDARD 23)

# 1) Пакеты
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(assimp REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(X11 REQUIRED)   

# 2) Внешние либы (target-based)

## stb
add_library(stb STATIC libs/stbi/stb_image.cc)
target_include_directories(stb PUBLIC libs/stbi)

## glad
add_library(glad STATIC libs/glad/src/glad.c)
target_include_directories(glad PUBLIC libs/glad/include)

## imgui (важно: объявляем ДО ссылок в disaster_sim)
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/libs/imgui)
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_compile_definitions(imgui PUBLIC
    IMGUI_IMPL_OPENGL_LOADER_GLAD
    IMGUI_DISABLE_OBSOLETE_FUNCTIONS
)
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw OpenGL::GL)

# Linux/X11: бекенд imgui_impl_glfw вызывает Xlib напрямую
if (X11_FOUND)
    target_link_libraries(imgui PUBLIC
        ${X11_X11_LIB}
        ${X11_Xcursor_LIB}
        ${X11_Xi_LIB}
        ${X11_Xrandr_LIB}
        ${X11_Xinerama_LIB}
    )
endif()

# 3) Источники приложения
file(GLOB_RECURSE SOURCE_FILES 
    src/main.cc
    src/core/*.cc
    src/data/*.cc
    src/core/simulation.cu
    src/graphics/*.cc
    src/graphics/OpenGL/*.cc
    src/scene/*.cc
    src/ui/*.cc
    src/app/*.cc
)

# 4) Исполняемый файл
add_executable(disaster_sim ${SOURCE_FILES})
set_source_files_properties(src/core/simulation.cu PROPERTIES LANGUAGE CUDA)

# <<< ВОТ СЮДА >>>
target_compile_definitions(disaster_sim PRIVATE GLFW_INCLUDE_NONE)

set_target_properties(disaster_sim PROPERTIES
    CUDA_RUNTIME_LIBRARY Shared
    CUDA_ARCHITECTURES "native"
)

# 5) Инклуды для приложения
target_include_directories(disaster_sim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    libs/glad/include/
    libs/stbi/
    libs/entt/
    ${CUDA_INCLUDE_DIRS}
)

# 6) Линковка приложения
target_link_libraries(disaster_sim
    ${CUDA_LIBRARIES}
    OpenGL::GL
    glfw
    glad
    assimp::assimp
    stb
    imgui
    yaml-cpp
)

# 7) Опции компиляции (CUDA)
target_compile_options(disaster_sim PRIVATE
    $<$<COMPILE_LANGUAGE:CUDA>:--expt-relaxed-constexpr>
    $<$<COMPILE_LANGUAGE:CUDA>:-arch=sm_60>
)

# Тесты - ТОЛЬКО CORE файлы
file(GLOB CORE_SOURCE_FILES src/core/*.cc
src/core/scene/*.cc)
add_library(core_for_tests STATIC ${CORE_SOURCE_FILES})

# ДОБАВИЛ ПУТИ К GLAD ДЛЯ CORE_FOR_TESTS
target_include_directories(core_for_tests PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include

    libs/glad/include/    # ← ДОБАВИЛ ЭТО
    libs/entt/ 
)

# ДОБАВИЛ GLAD В ЛИНКОВКУ ДЛЯ CORE_FOR_TESTS
target_link_libraries(core_for_tests PUBLIC glad)

find_package(GTest REQUIRED)
file(GLOB TEST_SRC_FILES "tests/*.cc")
add_executable(unit_tests ${TEST_SRC_FILES})

target_link_libraries(unit_tests
    core_for_tests
    GTest::gmock
    GTest::GTest
    GTest::Main
)

enable_testing()
add_test(NAME Tests COMMAND unit_tests)