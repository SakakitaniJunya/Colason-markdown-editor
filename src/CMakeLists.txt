file(GLOB_RECURSE COLASON_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

# Qt resources
set(COLASON_QRC "${CMAKE_SOURCE_DIR}/resources/colason.qrc")
if(EXISTS "${COLASON_QRC}")
    qt6_add_resources(COLASON_RESOURCES "${COLASON_QRC}")
endif()

# Platform application icon
if(APPLE)
    set(COLASON_APP_ICON "${CMAKE_SOURCE_DIR}/resources/icons/colason.icns")
    set_source_files_properties("${COLASON_APP_ICON}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
elseif(WIN32)
    set(COLASON_APP_ICON "${CMAKE_SOURCE_DIR}/resources/colason.rc")
endif()

add_executable(colason # WIN32 temporarily disabled for debugging
    ${COLASON_SOURCES}
    ${COLASON_RESOURCES}
    ${COLASON_APP_ICON}
)

if(APPLE)
    set_target_properties(colason PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "co.creanest.colason"
        MACOSX_BUNDLE_BUNDLE_NAME "Colason"
        MACOSX_BUNDLE_BUNDLE_VERSION "0.1.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1.0"
        MACOSX_BUNDLE_COPYRIGHT "© 2026 CreaNest"
        MACOSX_BUNDLE_ICON_FILE "colason.icns"
    )

    # macOS Release optimization for smaller binary
    if(CMAKE_BUILD_TYPE MATCHES Release)
        target_compile_options(colason PRIVATE -Os -flto)
        target_link_options(colason PRIVATE -flto)
    endif()
endif()

target_include_directories(colason PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(colason PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Optional Qt modules
if(Qt6WebEngineWidgets_FOUND)
    target_link_libraries(colason PRIVATE Qt6::WebEngineWidgets)
    target_compile_definitions(colason PRIVATE HAS_WEBENGINE=1)
endif()

if(Qt6WebChannel_FOUND)
    target_link_libraries(colason PRIVATE Qt6::WebChannel)
endif()

if(Qt6Svg_FOUND)
    target_link_libraries(colason PRIVATE Qt6::Svg)
endif()

if(Qt6PrintSupport_FOUND)
    target_link_libraries(colason PRIVATE Qt6::PrintSupport)
endif()

if(Qt6Concurrent_FOUND)
    target_link_libraries(colason PRIVATE Qt6::Concurrent)
endif()

if(spdlog_FOUND)
    target_link_libraries(colason PRIVATE spdlog::spdlog)
    target_compile_definitions(colason PRIVATE HAS_SPDLOG=1)
endif()

if(nlohmann_json_FOUND)
    target_link_libraries(colason PRIVATE nlohmann_json::nlohmann_json)
endif()

# Windows-specific
if(WIN32)
    target_link_libraries(colason PRIVATE ole32 oleaut32 shell32 dwmapi)
    target_compile_definitions(colason PRIVATE
        UNICODE _UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
    )
endif()

# Editor dist directory for development
target_compile_definitions(colason PRIVATE
    EDITOR_DIST_DIR="${EDITOR_DIST_DIR}"
)
