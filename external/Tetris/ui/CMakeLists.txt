# Localiza a SFML (2.5+). Instale os headers/libs da SFML no sistema antes de compilar a GUI.
# Linux (Debian/Ubuntu): sudo apt install libsfml-dev
# Linux (Fedora):        sudo dnf install SFML-devel
# Arch:                  sudo pacman -S sfml
# Windows: defina SFML_DIR para a pasta do SFMLConfig.cmake
find_package(SFML 2.5 COMPONENTS graphics window system audio REQUIRED)

if(NOT TARGET tetris_env)
  message(FATAL_ERROR "tetris_env target not found. Configure the build from the repository root so the AI modes are available.")
endif()

add_library(tetris_ui STATIC
  src/App.cpp
  src/Colors.cpp
  src/Config.cpp
  src/Hud.cpp
  src/Renderer.cpp
)

target_include_directories(tetris_ui
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# A UI depende do motor e da SFML
target_link_libraries(tetris_ui
  PUBLIC
    tetris_engine
    tetris_env
    sfml-graphics sfml-window sfml-system sfml-audio
)

target_compile_features(tetris_ui PUBLIC cxx_std_20)
