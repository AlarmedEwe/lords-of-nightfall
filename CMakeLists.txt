cmake_minimum_required(VERSION 3.20)
project(LordsOfNightfall LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ==============================================================================
# Build Configurations
# ==============================================================================
# CMake has Debug and Release by default. We add "Development" as a custom config.
# Debug: símbolos completos, sem otimização, ferramentas de dev
# Development: otimizações parciais, símbolos, ferramentas de dev
# Release: otimização máxima, sem símbolos, sem ferramentas de dev

set(CMAKE_CONFIGURATION_TYPES "Debug;Development;Release" CACHE STRING "" FORCE)

# Development herda de RelWithDebInfo como base
set(CMAKE_CXX_FLAGS_DEVELOPMENT "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}" CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_DEVELOPMENT "${CMAKE_C_FLAGS_RELWITHDEBINFO}" CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_DEVELOPMENT "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO}" CACHE STRING "" FORCE)

# Default build type para single-config generators (Make, Ninja)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Development" CACHE STRING "Build type" FORCE)
endif()

# ==============================================================================
# Output Directories
# ==============================================================================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Build/${CMAKE_BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Build/${CMAKE_BUILD_TYPE})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/Build/Intermediate/${CMAKE_BUILD_TYPE})

# Intermediários (objetos) vão para Build/Intermediate/<Config>/
set(CMAKE_OBJECT_PATH_MAX 512)

# ==============================================================================
# Compile Definitions per config
# ==============================================================================
add_compile_definitions(
    $<$<CONFIG:Debug>:LON_DEBUG=1>
    $<$<CONFIG:Debug>:LON_DEVELOPMENT=1>
    $<$<CONFIG:Development>:LON_DEVELOPMENT=1>
    $<$<CONFIG:Release>:LON_RELEASE=1>
)

# ==============================================================================
# ThirdParty: raylib
# ==============================================================================
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
add_subdirectory(Game/ThirdParty/raylib)

# ==============================================================================
# ThirdParty: Jolt Physics
# ==============================================================================
set(TARGET_HELLO_WORLD OFF CACHE BOOL "" FORCE)
set(TARGET_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(TARGET_PERFORMANCE_TEST OFF CACHE BOOL "" FORCE)
set(TARGET_SAMPLES OFF CACHE BOOL "" FORCE)
set(TARGET_VIEWER OFF CACHE BOOL "" FORCE)
set(TARGET_SHADERS OFF CACHE BOOL "" FORCE)
set(JPH_USE_DX12 OFF CACHE BOOL "" FORCE)
set(JPH_USE_VK OFF CACHE BOOL "" FORCE)
set(JPH_USE_MTL OFF CACHE BOOL "" FORCE)
add_subdirectory(Game/ThirdParty/JoltPhysics/Build)

# ==============================================================================
# ==============================================================================
# ThirdParty: Dear ImGui & rlImGui (compilado como library estática)
# ==============================================================================
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/Game/ThirdParty/imgui)
set(RLIMGUI_DIR ${CMAKE_SOURCE_DIR}/Game/ThirdParty/rlImGui)
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${RLIMGUI_DIR}/rlImGui.cpp
)
target_include_directories(imgui PUBLIC 
    ${IMGUI_DIR}
    ${RLIMGUI_DIR}
    ${CMAKE_SOURCE_DIR}/Game/ThirdParty/raylib/src
)

# ==============================================================================
# Game Executable
# ==============================================================================
file(GLOB_RECURSE GAME_SOURCES "Game/Source/*.cpp")

add_executable(${PROJECT_NAME} ${GAME_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    Game/Source
    Game/ThirdParty/JoltPhysics
    Game/ThirdParty/imgui
    Game/ThirdParty/rlImGui
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    raylib
    Jolt
    imgui
)

# ==============================================================================
# Asset Cooker (build event)
# ==============================================================================
find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(ASSET_OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Assets)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/Tools/AssetCooker.py ${ASSET_OUTPUT_DIR}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "[AssetCooker] Processando assets..."
)

# Cria o diretório Saved para ferramentas de runtime (Debug/Development)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Saved
    COMMENT "Criando diretório Saved para runtime..."
)
