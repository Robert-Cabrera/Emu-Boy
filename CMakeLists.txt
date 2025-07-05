cmake_minimum_required(VERSION 3.10)
project(GameboyEmulator)

# Set the sources (add libs/tinyfiledialogs.c)
file(GLOB SOURCES
    "${CMAKE_SOURCE_DIR}/sources/*.cpp"
)

# Include headers 
include_directories("${CMAKE_SOURCE_DIR}/headers" "${CMAKE_SOURCE_DIR}/libs")

# Add the executable
add_executable(emu ${SOURCES})

# Link local SDL2 libraries
# Find SDL2 using pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)

# Include SDL2 directories
include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS})

# Link against SDL2 libraries
target_link_libraries(emu PRIVATE ${SDL2_LIBRARIES} ${SDL2_TTF_LIBRARIES} ${SDL2_IMAGE_LIBRARIES})

# Set the icon (depends on your windowing code, here's a general approach)
configure_file(assets/icon.png ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

# Custom command for embedding the icon if necessary (depends on your environment)
# This typically requires platform-specific code to set the icon using SDL2 or similar

