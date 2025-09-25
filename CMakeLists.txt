cmake_minimum_required(VERSION 3.20)
project(tetris_ai_agents LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Desliga apps/GUI do submódulo (usam as mesmas cache vars do projeto A)
set(BUILD_GUI OFF CACHE BOOL "Disable GUI from submodule" FORCE)
set(BUILD_CLI OFF CACHE BOOL "Disable CLI from submodule" FORCE)

add_subdirectory(external/tetris)

# Seu runner/agents vão linkar só na engine do submódulo:
add_executable(ai_runner apps/ai_runner.cpp)      # (stub por enquanto)
target_link_libraries(ai_runner PRIVATE tetris_engine)
