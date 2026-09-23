# Both usage strings have to survive into the built bundle, not merely exist in
# MacOSXBundleInfo.plist.in. macOS kills a process that asks for the microphone
# or for speech recognition when the responsible application's Info.plist does
# not describe why it wants them, and when Mudlet is that application - the
# ordinary case for a player, who starts it from Finder - the process killed is
# Mudlet, with no Lua error and nothing in its own log to say so. A build-system
# change that drops either key would reach players rather than developers, so
# this reads them back out of the bundle that was built.
#
# Run against $<TARGET_BUNDLE_DIR:mudlet_executable>/Contents/Info.plist, passed
# in as BUNDLE_PLIST.

if(NOT DEFINED BUNDLE_PLIST)
    message(FATAL_ERROR "BUNDLE_PLIST was not set, so there is no bundle to check")
endif()

if(NOT EXISTS "${BUNDLE_PLIST}")
    message(FATAL_ERROR "no Info.plist at ${BUNDLE_PLIST} - the bundle was not built")
endif()

# Read through plutil rather than by searching the file's text. macOS reads the
# top level of the plist, and a text search cannot tell that from a key sitting
# inside one of the nested dictionaries this plist has (CFBundleURLTypes), or
# from one left behind in an XML comment - both of which would satisfy a search
# while macOS still saw nothing and killed the process.
if(NOT EXISTS "/usr/bin/plutil")
    message(FATAL_ERROR "/usr/bin/plutil is missing, so the bundle's Info.plist cannot be read the way macOS reads it")
endif()

foreach(usageKey NSMicrophoneUsageDescription NSSpeechRecognitionUsageDescription)
    # -extract takes a key path from the root, so a nested key of the same name
    # is not found by this and a commented-out one does not exist at all.
    execute_process(
        COMMAND /usr/bin/plutil -extract ${usageKey} xml1 -o - "${BUNDLE_PLIST}"
        RESULT_VARIABLE extractResult
        OUTPUT_VARIABLE extractedValue
        ERROR_VARIABLE extractError)

    if(NOT extractResult EQUAL 0)
        message(FATAL_ERROR "${usageKey} is missing from the top level of ${BUNDLE_PLIST}: macOS would kill Mudlet when speech asks for this permission (${extractError})")
    endif()

    # It has to be a <string>. macOS wants text to show the player in the
    # permission dialog, and anything else - an <integer>, a <true/>, a nested
    # container - is a key it cannot use, which it treats as no key at all.
    if(NOT extractedValue MATCHES "<string>([^<]*)</string>")
        message(FATAL_ERROR "${usageKey} in ${BUNDLE_PLIST} is not a string, which macOS treats as no key at all")
    endif()

    # Present but blank is the same to macOS as absent, and whitespace is blank.
    if(NOT "${CMAKE_MATCH_1}" MATCHES "[^ \t\r\n]")
        message(FATAL_ERROR "${usageKey} in ${BUNDLE_PLIST} has no text, which macOS treats as no key at all")
    endif()
endforeach()

message(STATUS "both macOS usage strings are present in ${BUNDLE_PLIST}")
