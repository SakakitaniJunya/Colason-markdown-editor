cmake_minimum_required(VERSION 3.25)

# vcpkg toolchain (must be before project())
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
endif()

project(Colason VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Qt auto-tools
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Find Qt6
find_package(Qt6 COMPONENTS Core Gui Widgets QUIET)
if(Qt6Core_FOUND AND Qt6Gui_FOUND AND Qt6Widgets_FOUND)
    set(Qt6_FOUND TRUE)
endif()

# WebEngine depends on Quick/Qml/Network/OpenGL/Positioning — find them first
find_package(Qt6 COMPONENTS Network OpenGL Qml Quick QuickWidgets Positioning QUIET)
find_package(Qt6 COMPONENTS WebEngineCore WebEngineWidgets WebChannel Svg PrintSupport Concurrent QUIET)

message(STATUS "Qt6WebEngineWidgets_FOUND: ${Qt6WebEngineWidgets_FOUND}")
message(STATUS "Qt6WebChannel_FOUND: ${Qt6WebChannel_FOUND}")
message(STATUS "Qt6Quick_FOUND: ${Qt6Quick_FOUND}")

# Find other dependencies
find_package(spdlog CONFIG QUIET)
find_package(nlohmann_json CONFIG QUIET)

# Editor web assets directory
set(EDITOR_DIST_DIR "${CMAKE_SOURCE_DIR}/editor/dist")

# Source files
add_subdirectory(src)

# Tests
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS AND EXISTS "${CMAKE_SOURCE_DIR}/tests/CMakeLists.txt")
    enable_testing()
    find_package(GTest CONFIG QUIET)
    if(GTest_FOUND)
        add_subdirectory(tests)
    endif()
endif()
