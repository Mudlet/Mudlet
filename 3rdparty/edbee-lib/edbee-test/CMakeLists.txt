# Should be called before PROJECT.
CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

IF(POLICY CMP0020)
  CMAKE_POLICY(SET CMP0020 NEW)
ENDIF()

PROJECT(edbee-test)

SET(SOURCES
   edbee/commands/replaceselectioncommandtest.cpp 
   edbee/models/textrangetest.cpp 
    edbee/models/textdocumenttest.cpp 
    edbee/models/textbuffertest.cpp 
   edbee/models/textlinedatatest.cpp 
   edbee/util/gapvectortest.cpp 
   edbee/util/lineoffsetvectortest.cpp 
   main.cpp 
    edbee/util/lineendingtest.cpp 
    edbee/textdocumentserializertest.cpp 
    edbee/io/tmlanguageparsertest.cpp 
    edbee/util/regexptest.cpp 
    edbee/models/textdocumentscopestest.cpp 
    edbee/models/textundostacktest.cpp 
    edbee/util/cascadingqvariantmaptest.cpp 
    edbee/models/textsearchertest.cpp 
    edbee/commands/duplicatecommandtest.cpp 
    edbee/commands/newlinecommandtest.cpp 
    edbee/util/utiltest.cpp 
    edbee/lexers/grammartextlexertest.cpp 
    edbee/commands/removecommandtest.cpp 
    edbee/models/changes/linedatalistchangetest.cpp 
    edbee/models/changes/textchangetest.cpp 
    edbee/models/changes/mergablechangegrouptest.cpp 
    edbee/util/rangesetlineiteratortest.cpp 
    edbee/models/dynamicvariablestest.cpp 
    edbee/util/rangelineiteratortest.cpp
)

SET(HEADERS
   edbee/commands/replaceselectioncommandtest.h 
   edbee/models/textrangetest.h 
    edbee/models/textdocumenttest.h 
    edbee/models/textbuffertest.h 
   edbee/models/textlinedatatest.h 
   edbee/util/gapvectortest.h 
   edbee/util/lineoffsetvectortest.h 
    edbee/util/lineendingtest.h 
    edbee/textdocumentserializertest.h 
    edbee/io/tmlanguageparsertest.h 
    edbee/util/regexptest.h 
    edbee/models/textdocumentscopestest.h 
    edbee/models/textundostacktest.h 
    edbee/util/cascadingqvariantmaptest.h 
    edbee/models/textsearchertest.h 
    edbee/commands/duplicatecommandtest.h 
    edbee/commands/newlinecommandtest.h 
    edbee/util/utiltest.h 
    edbee/lexers/grammartextlexertest.h 
    edbee/commands/removecommandtest.h 
    edbee/models/changes/linedatalistchangetest.h 
    edbee/models/changes/textchangetest.h 
    edbee/models/changes/mergablechangegrouptest.h 
    edbee/util/rangesetlineiteratortest.h 
    edbee/models/dynamicvariablestest.h 
    edbee/util/rangelineiteratortest.h
)

FIND_PACKAGE(Qt5Core REQUIRED)
FIND_PACKAGE(Qt5UiTools REQUIRED)
FIND_PACKAGE(Qt5Widgets REQUIRED)

add_subdirectory(../vendor/qslog/ qslog)

ADD_DEFINITIONS(
    ${Qt5Core_DEFINITIONS}
    ${Qt5UiTools_DEFINTIONS}
    ${Qt5Widgets_DEFINTIONS}
)

SET(CMAKE_INCLUDE_CURRENT_DIR ON)
INCLUDE_DIRECTORIES(
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5UiTools_INCLUDE_DIRS}
    ${Qt5Widgets_INCLUDE_DIRS}
    ../edbee-lib/
    ${QS_LOG_INCLUDE_DIRS}
)

set(CMAKE_AUTOMOC ON)

ADD_EXECUTABLE(edbee-test
  ${SOURCES} ${HEADERS} ${QS_LOG_SOURCES} ${QS_LOG_HEADERS}
)

TARGET_LINK_LIBRARIES(edbee-test edbee-lib ${Qt5Core_LIBRARIES}
    ${Qt5Widgets_LIBRARIES} ${Qt5UiTools_LIBRARIES})

SET_PROPERTY(TARGET edbee-test PROPERTY CXX_STANDARD 11)
