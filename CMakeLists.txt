cmake_minimum_required(VERSION 3.12)
project(MacroWise)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(X11 REQUIRED x11)

# Check for ncurses
find_package(Curses REQUIRED)

# Check for xdotool using find_program instead
find_program(XDOTOOL_EXECUTABLE xdotool)
if(NOT XDOTOOL_EXECUTABLE)
    message(WARNING "xdotool not found - cursor position features will be limited")
endif()

# Filesystem compatibility
include(CheckCXXSourceCompiles)
check_cxx_source_compiles("
#include <filesystem>
int main() {
    std::filesystem::path p = \"/tmp\";
    return 0;
}
" HAS_STD_FILESYSTEM)

if(NOT HAS_STD_FILESYSTEM)
    check_cxx_source_compiles("
    #include <experimental/filesystem>
    int main() {
        std::experimental::filesystem::path p = \"/tmp\";
        return 0;
    }
    " HAS_EXPERIMENTAL_FILESYSTEM)
    
    if(HAS_EXPERIMENTAL_FILESYSTEM)
        add_compile_definitions(USE_EXPERIMENTAL_FILESYSTEM)
        set(FILESYSTEM_LIB stdc++fs)
    else()
        message(FATAL_ERROR "No filesystem support found")
    endif()
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${X11_INCLUDE_DIRS}
    ${CURSES_INCLUDE_DIRS}
)

# Source files
set(SOURCES
    src/main.cpp
    src/MacroRecorder.cpp
    src/UInputDevice.cpp
    src/utils.cpp
    src/Interface.cpp
)

# Executable
add_executable(MacroWise ${SOURCES})

# Link libraries
target_link_libraries(MacroWise
    ${X11_LIBRARIES}
    ${FILESYSTEM_LIB}
    ${CURSES_LIBRARIES}
    pthread
)

# Install target
install(TARGETS MacroWise DESTINATION bin)