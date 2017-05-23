# Should be called before PROJECT.
CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

IF(POLICY CMP0020)
  CMAKE_POLICY(SET CMP0020 NEW)
ENDIF()

PROJECT(edbee-lib)

SET(SOURCES
   edbee/util/mem/debug_new.cpp 
   edbee/util/mem/debug_allocs.cpp 
   edbee/util/simpleprofiler.cpp 
   edbee/util/textcodecdetector.cpp 
   edbee/util/lineending.cpp 
   edbee/texteditorwidget.cpp 
   edbee/views/textrenderer.cpp 
   edbee/models/textdocument.cpp 
   edbee/models/chardocument/chartextdocument.cpp 
   edbee/models/texteditorconfig.cpp 
   edbee/models/texteditorkeymap.cpp 
   edbee/models/textundostack.cpp 
   edbee/views/textcaretcache.cpp 
   edbee/models/textlexer.cpp 
   edbee/models/textrange.cpp 
   edbee/views/textselection.cpp 
   edbee/models/textdocumentscopes.cpp 
   edbee/lexers/grammartextlexer.cpp 
   edbee/util/gapvector.h 
   edbee/util/lineoffsetvector.cpp 
   edbee/models/textlinedata.cpp 
   edbee/models/textbuffer.cpp 
   edbee/models/chardocument/chartextbuffer.cpp 
   edbee/texteditorcontroller.cpp 
   edbee/texteditorcommand.cpp 
   edbee/commands/selectioncommand.cpp 
   edbee/commands/undocommand.cpp 
   edbee/commands/redocommand.cpp 
   edbee/commands/replaceselectioncommand.cpp 
   edbee/commands/copycommand.cpp 
   edbee/commands/cutcommand.cpp 
   edbee/commands/pastecommand.cpp 
   edbee/io/textdocumentserializer.cpp 
   edbee/util/test.cpp 
   edbee/util/textcodec.cpp 
   edbee/io/tmlanguageparser.cpp 
   edbee/commands/debugcommand.cpp 
   edbee/util/regexp.cpp 
   edbee/io/tmthemeparser.cpp 
   edbee/io/baseplistparser.cpp 
   edbee/io/jsonparser.cpp 
   edbee/models/textgrammar.cpp 
   edbee/models/texteditorcommandmap.cpp 
   edbee/views/components/texteditorcomponent.cpp 
   edbee/views/components/texteditorrenderer.cpp 
   edbee/views/components/textmargincomponent.cpp 
   edbee/views/texttheme.cpp 
   edbee/views/texteditorscrollarea.cpp 
   edbee/models/textsearcher.cpp 
   edbee/commands/findcommand.cpp 
   edbee/io/keymapparser.cpp 
   edbee/commands/tabcommand.cpp 
   edbee/edbee.cpp 
   edbee/models/textdocumentfilter.cpp 
   edbee/util/cascadingqvariantmap.cpp 
   edbee/commands/duplicatecommand.cpp 
   edbee/commands/newlinecommand.cpp 
   edbee/util/util.cpp 
   edbee/commands/removecommand.cpp 
   edbee/models/change.cpp 
   edbee/models/changes/abstractrangedchange.cpp 
   edbee/models/changes/linedatalistchange.cpp 
   edbee/models/changes/linedatachange.cpp 
   edbee/models/changes/selectionchange.cpp 
   edbee/models/changes/textchange.cpp 
   edbee/models/changes/textchangewithcaret.cpp 
   edbee/models/changes/mergablechangegroup.cpp 
   edbee/commands/commentcommand.cpp 
   edbee/util/rangesetlineiterator.cpp 
   edbee/models/dynamicvariables.cpp 
   edbee/util/rangelineiterator.cpp 
   edbee/data/factorykeymap.cpp 
   edbee/data/factorycommandmap.cpp
)

SET(HEADERS
   edbee/util/logging.h 
   edbee/util/mem/debug_new.h 
   edbee/util/mem/debug_allocs.h 
   edbee/util/simpleprofiler.h 
   edbee/util/textcodecdetector.h 
   edbee/util/lineending.h 
   edbee/texteditorwidget.h 
   edbee/views/textrenderer.h 
   edbee/models/textdocument.h 
   edbee/models/chardocument/chartextdocument.h 
   edbee/models/texteditorconfig.h 
   edbee/models/texteditorkeymap.h 
   edbee/models/textundostack.h 
   edbee/texteditorcontroller.h 
   edbee/views/textcaretcache.h 
   edbee/models/textlexer.h 
   edbee/models/textrange.h 
   edbee/views/textselection.h 
   edbee/models/textdocumentscopes.h 
   edbee/lexers/grammartextlexer.h 
   edbee/util/lineoffsetvector.h 
   edbee/models/textlinedata.h 
   edbee/models/textbuffer.h 
   edbee/models/chardocument/chartextbuffer.h 
   edbee/texteditorcommand.h 
   edbee/commands/selectioncommand.h 
   edbee/commands/undocommand.h 
   edbee/commands/redocommand.h 
   edbee/commands/replaceselectioncommand.h 
   edbee/commands/copycommand.h 
   edbee/commands/cutcommand.h 
   edbee/commands/pastecommand.h 
   edbee/models/textdocumentfilter.h 
   edbee/debug.h 
   edbee/io/textdocumentserializer.h 
   edbee/util/test.h 
   edbee/util/textcodec.h 
   edbee/io/tmlanguageparser.h 
   edbee/commands/debugcommand.h 
   edbee/util/regexp.h 
   edbee/io/tmthemeparser.h 
   edbee/io/baseplistparser.h 
   edbee/io/jsonparser.h 
   edbee/models/textgrammar.h 
   edbee/models/texteditorcommandmap.h 
   edbee/views/components/texteditorcomponent.h 
   edbee/views/components/texteditorrenderer.h 
   edbee/views/components/textmargincomponent.h 
   edbee/views/texttheme.h 
   edbee/views/texteditorscrollarea.h 
   edbee/models/textsearcher.h 
   edbee/commands/findcommand.h 
   edbee/io/keymapparser.h 
   edbee/commands/tabcommand.h 
   edbee/edbee.h 
   edbee/util/cascadingqvariantmap.h 
   edbee/commands/duplicatecommand.h 
   edbee/commands/newlinecommand.h 
   edbee/util/util.h 
   edbee/commands/removecommand.h 
   edbee/models/change.h 
   edbee/models/changes/abstractrangedchange.h 
   edbee/models/changes/linedatalistchange.h 
   edbee/models/changes/linedatachange.h 
   edbee/models/changes/selectionchange.h 
   edbee/models/changes/textchange.h 
   edbee/models/changes/textchangewithcaret.h 
   edbee/models/changes/mergablechangegroup.h 
   edbee/commands/commentcommand.h 
   edbee/util/rangesetlineiterator.h 
   edbee/models/dynamicvariables.h 
   edbee/util/rangelineiterator.h 
   edbee/data/factorykeymap.h 
   edbee/data/factorycommandmap.h
) 

FIND_PACKAGE(Qt5Core REQUIRED)
FIND_PACKAGE(Qt5UiTools REQUIRED)
FIND_PACKAGE(Qt5Widgets REQUIRED)

IF(Qt5_POSITION_INDEPENDENT_CODE)
  SET(CMAKE_POSITION_INDEPENDENT_CODE ON)
ENDIF()

set(CMAKE_AUTOMOC ON)

add_subdirectory(../vendor/qslog/ qslog)
add_subdirectory(../vendor/onig/ onig)

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
    ${QS_LOG_INCLUDE_DIRS}
    ${ONIG_INCLUDE_DIRS}
)

ADD_LIBRARY(edbee-lib STATIC
  ${SOURCES} ${HEADERS} ${QS_LOG_SOURCES} ${QS_LOG_HEADERS} ${ONIG_SOURCES}
  ${ONIG_HEADERS})

TARGET_INCLUDE_DIRECTORIES(edbee-lib PUBLIC
  ${Qt5Core_INCLUDE_DIRS}
  ${Qt5UiTools_INCLUDE_DIRS}
  ${Qt5Widgets_INCLUDE_DIRS}
  ${QS_LOG_INCLUDE_DIRS}
  ${ONIG_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

SET_PROPERTY(TARGET edbee-lib PROPERTY CXX_STANDARD 11)
