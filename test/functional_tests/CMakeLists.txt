# Functional tests optionally enable sanitizers when building from the full Mudlet build tree.
# When building tests standalone (cmake -S test), the 3rdparty sanitizer helpers may not be available.
if(NOT WIN32)
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../../src/cmake/EnableSanitizers.cmake" AND EXISTS "${CMAKE_CURRENT_LIST_DIR}/../3rdparty/cmake-scripts/sanitizers.cmake")
        include(${CMAKE_CURRENT_LIST_DIR}/../../src/cmake/EnableSanitizers.cmake)
    endif()
endif()

set(FUNCTIONAL_TEST_SOURCES
    TelnetTextDisplayedTest.cpp
    TelnetBenchmark.cpp
    TOscTerminatorTest.cpp
)

set(FUNCTIONAL_TEST_UTILS
    TelnetServerStub.cpp
)

foreach(test_file ${FUNCTIONAL_TEST_SOURCES})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_executable(${test_name} ${test_file} ${FUNCTIONAL_TEST_UTILS})
    add_dependencies(${test_name} ${LIB_MUDLET_TARGET})
    target_link_libraries(${test_name} PRIVATE Qt6::Test ${LIB_MUDLET_TARGET})
    set_target_properties(${test_name} PROPERTIES ENABLE_EXPORTS ON)
    add_test(NAME ${test_name} COMMAND $<TARGET_FILE:${test_name}>)
    set_tests_properties(${test_name} PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;ASAN_OPTIONS=detect_leaks=0"
        LABELS "functional"
        TIMEOUT 60  # seconds
    )
endforeach()
