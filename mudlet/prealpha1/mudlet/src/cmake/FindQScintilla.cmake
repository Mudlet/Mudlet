# - Try to find the QScintilla2 includes and library
# which defines
#
# QSCINTILLA_FOUND - system has QScintilla2
# QSCINTILLA_INCLUDE_DIR - where to find qextscintilla.h
# QSCINTILLA_LIBRARIES - the libraries to link against to use QScintilla
# QSCINTILLA_LIBRARY - where to find the QScintilla library (not for general use)

# copyright (c) 2007 Thomas Moenicke thomas.moenicke@kdemail.net
#
# Redistribution and use is allowed according to the terms of the BSD license.

IF(NOT QT4_FOUND)
    INCLUDE(FindQt4)
ENDIF(NOT QT4_FOUND)

SET(QSCINTILLA_FOUND "NO")

IF(QT4_FOUND)
    FIND_PATH(QSCINTILLA_INCLUDE_DIR qsciglobal.h
    "${QT_INCLUDE_DIR}/Qsci"
    )

    SET(QSCINTILLA_NAMES ${QSCINTILLA_NAMES} qscintilla2 libqscintilla2)
    FIND_LIBRARY(QSCINTILLA_LIBRARY
        NAMES ${QSCINTILLA_NAMES}
        PATHS ${QT_LIBRARY_DIR}
    )

    IF (QSCINTILLA_LIBRARY)

        SET(QSCINTILLA_LIBRARIES ${QSCINTILLA_LIBRARY})
        SET(QSCINTILLA_FOUND "YES")

        IF (CYGWIN)
            IF(BUILD_SHARED_LIBS)
            # No need to define QSCINTILLA_USE_DLL here, because it's default for Cygwin.
            ELSE(BUILD_SHARED_LIBS)
            SET (QSCINTILLA_DEFINITIONS -DQSCINTILLA_STATIC)
            ENDIF(BUILD_SHARED_LIBS)
        ENDIF (CYGWIN)

    ENDIF (QSCINTILLA_LIBRARY)
ENDIF(QT4_FOUND)

IF (QSCINTILLA_FOUND)
  IF (NOT QSCINTILLA_FIND_QUIETLY)
    MESSAGE(STATUS "Found QScintilla2: ${QSCINTILLA_LIBRARY}")
  ENDIF (NOT QSCINTILLA_FIND_QUIETLY)
ELSE (QSCINTILLA_FOUND)
  IF (QSCINTILLA_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Could not find QScintilla library")
  ENDIF (QSCINTILLA_FIND_REQUIRED)
ENDIF (QSCINTILLA_FOUND)

MARK_AS_ADVANCED(QSCINTILLA_INCLUDE_DIR QSCINTILLA_LIBRARY)
