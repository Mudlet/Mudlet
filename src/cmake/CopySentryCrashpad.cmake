# Script to find and copy Sentry crashpad files at build time
# This runs as part of the POST_BUILD step for the mudlet executable

if(NOT SENTRY_BUILD_DIR)
  message(FATAL_ERROR "SENTRY_BUILD_DIR not defined")
endif()

if(NOT TARGET_DIR)
  message(FATAL_ERROR "TARGET_DIR not defined")
endif()

message(STATUS "Searching for Sentry crashpad files in: ${SENTRY_BUILD_DIR}")

# Find crashpad_handler executable
file(GLOB_RECURSE CRASHPAD_HANDLER_FILES 
  "${SENTRY_BUILD_DIR}/*crashpad_handler"
  "${SENTRY_BUILD_DIR}/*crashpad_handler.exe"
)

# Find crashpad_wer.dll (Windows only)
file(GLOB_RECURSE CRASHPAD_WER_FILES 
  "${SENTRY_BUILD_DIR}/*crashpad_wer.dll"
)

set(FILES_COPIED 0)

# Copy crashpad_handler
foreach(FILE ${CRASHPAD_HANDLER_FILES})
  get_filename_component(FILENAME ${FILE} NAME)
  # Skip if it's in a CMakeFiles directory (intermediate files)
  string(FIND "${FILE}" "CMakeFiles" POS)
  if(POS EQUAL -1)
    message(STATUS "Copying ${FILENAME} to ${TARGET_DIR}")
    file(COPY "${FILE}" DESTINATION "${TARGET_DIR}")
    math(EXPR FILES_COPIED "${FILES_COPIED} + 1")
    break() # Only copy the first valid one found
  endif()
endforeach()

# Copy crashpad_wer.dll if found
foreach(FILE ${CRASHPAD_WER_FILES})
  get_filename_component(FILENAME ${FILE} NAME)
  string(FIND "${FILE}" "CMakeFiles" POS)
  if(POS EQUAL -1)
    message(STATUS "Copying ${FILENAME} to ${TARGET_DIR}")
    file(COPY "${FILE}" DESTINATION "${TARGET_DIR}")
    math(EXPR FILES_COPIED "${FILES_COPIED} + 1")
    break()
  endif()
endforeach()

if(FILES_COPIED EQUAL 0)
  message(WARNING "No Sentry crashpad files found in ${SENTRY_BUILD_DIR}")
else()
  message(STATUS "Copied ${FILES_COPIED} Sentry crashpad file(s)")
endif()
