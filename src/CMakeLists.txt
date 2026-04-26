file(GLOB_RECURSE COLASON_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

# Qt resources
set(COLASON_QRC "${CMAKE_SOURCE_DIR}/resources/colason.qrc")
if(EXISTS "${COLASON_QRC}")
    qt6_add_resources(COLASON_RESOURCES "${COLASON_QRC}")
endif()

add_executable(colason # WIN32 temporarily disabled for debugging
    ${COLASON_SOURCES}
    ${COLASON_RESOURCES}
)

if(APPLE)
    set_target_properties(colason PROPERTIES MACOSX_BUNDLE TRUE)
    
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
