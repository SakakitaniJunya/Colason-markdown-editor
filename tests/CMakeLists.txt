file(GLOB_RECURSE TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

if(TEST_SOURCES AND GTest_FOUND)
    add_executable(colason_tests ${TEST_SOURCES})
    target_link_libraries(colason_tests PRIVATE
        GTest::gtest GTest::gtest_main
        Qt6::Core
    )
    if(nlohmann_json_FOUND)
        target_link_libraries(colason_tests PRIVATE nlohmann_json::nlohmann_json)
    endif()
    target_include_directories(colason_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/src
    )
    include(GoogleTest)
    gtest_discover_tests(colason_tests)
endif()
