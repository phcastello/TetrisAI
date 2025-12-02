cmake_minimum_required(VERSION 3.20)

project(tetris_ai_env LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(external/Tetris/engine)

add_library(tetris_env
  src/tetris_env/TetrisEnv.cpp
  src/tetris_env/RandomAgent.cpp
  src/tetris_env/MctsRolloutAgent.cpp
  src/tetris_env/MctsConfig.cpp
  src/tetris_env/RunLogging.cpp
  src/tetris_env/GreedyAgent.cpp
)
target_include_directories(tetris_env
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(tetris_env
  PUBLIC
    tetris_engine
)
target_compile_features(tetris_env PUBLIC cxx_std_20)
if(MSVC)
  target_compile_options(tetris_env PRIVATE /W4 /permissive-)
else()
  target_compile_options(tetris_env PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(tetris_env_test apps/tetris_env_test.cpp)
target_link_libraries(tetris_env_test PRIVATE tetris_env)
target_compile_features(tetris_env_test PRIVATE cxx_std_20)
if(MSVC)
  target_compile_options(tetris_env_test PRIVATE /W4 /permissive-)
else()
  target_compile_options(tetris_env_test PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(tetris_batch_runner apps/tetris_batch_runner.cpp)
target_link_libraries(tetris_batch_runner PRIVATE tetris_env)
target_compile_features(tetris_batch_runner PRIVATE cxx_std_20)
if(MSVC)
  target_compile_options(tetris_batch_runner PRIVATE /W4 /permissive-)
else()
  target_compile_options(tetris_batch_runner PRIVATE -Wall -Wextra -Wpedantic)
endif()

option(BUILD_TETRIS_GUI "Build the SFML GUI Tetris app with AI modes" OFF)
if(BUILD_TETRIS_GUI)
  add_subdirectory(external/Tetris/ui)
  add_executable(tetris_gui external/Tetris/apps/tetris_gui.cpp)
  target_link_libraries(tetris_gui PRIVATE tetris_ui tetris_env)
  target_compile_features(tetris_gui PRIVATE cxx_std_20)
endif()
