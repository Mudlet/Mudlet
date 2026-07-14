if(NOT WIN32)
    include(${CMAKE_SOURCE_DIR}/src/cmake/EnableSanitizers.cmake)
endif()

set(FUNCTIONAL_TEST_SOURCES
    TelnetTextDisplayedTest.cpp
    TelnetSubnegotiationTest.cpp
    TelnetBenchmark.cpp
    ResetProfileTest.cpp
    TOscTest.cpp
    TUserWindowTest.cpp
    TriggerEditorTest.cpp
    TFeedTriggersRecursionTest.cpp
    dlgTriggerEditorUndoRedoTest.cpp
    TDiscordModeTest.cpp
    EnableDisableByNameTest.cpp
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

# ResetProfileTest creates a full profile per test method (30 tests),
# so it needs a longer timeout than the default 60s
set_tests_properties(ResetProfileTest PROPERTIES TIMEOUT 300)

# Undo/redo tests need a longer timeout due to large batch operations
set_tests_properties(dlgTriggerEditorUndoRedoTest PROPERTIES TIMEOUT 300)
