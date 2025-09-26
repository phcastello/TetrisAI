cmake_minimum_required(VERSION 3.20)
project(tetris_ai_agents LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Desliga apps/GUI do submódulo (usam as mesmas cache vars do projeto A)
set(BUILD_GUI OFF CACHE BOOL "Disable GUI from submodule" FORCE)
set(BUILD_CLI OFF CACHE BOOL "Disable CLI from submodule" FORCE)

add_subdirectory(external/Tetris)

add_library(env_tetris STATIC
    adapters/tetris_game/GameEnv.cpp
)

target_include_directories(env_tetris
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/adapters
        ${CMAKE_CURRENT_SOURCE_DIR}/sdk/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/Tetris/engine/include
)

target_link_libraries(env_tetris
    PUBLIC
        tetris_engine
)

add_executable(ai_runner apps/ai_runner.cpp)      # (stub por enquanto)
target_link_libraries(ai_runner PRIVATE env_tetris)
