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

file(READ "${BUNDLE_PLIST}" plistContents)

foreach(usageKey NSMicrophoneUsageDescription NSSpeechRecognitionUsageDescription)
    string(FIND "${plistContents}" "<key>${usageKey}</key>" keyPosition)
    if(keyPosition EQUAL -1)
        message(FATAL_ERROR "${usageKey} is missing from ${BUNDLE_PLIST}: macOS would kill Mudlet when speech asks for this permission")
    endif()

    # The value that follows the key, whatever element it turns out to be. Taken
    # to the end of the file rather than a fixed window, so the length of the
    # description cannot decide whether this test can see it.
    string(SUBSTRING "${plistContents}" ${keyPosition} -1 keyAndRest)

    # It has to be a <string>. macOS wants text to show the player in the
    # permission dialog, and anything else - an <integer>, a <true/>, an element
    # left half-written - is a key it cannot use, which it treats as no key at
    # all and kills the process over.
    if(NOT keyAndRest MATCHES "^<key>${usageKey}</key>[ \t\r\n]*<string>([^<]*)</string>")
        message(FATAL_ERROR "${usageKey} in ${BUNDLE_PLIST} is not followed by a <string> value, which macOS treats as no key at all")
    endif()

    # Present but blank is the same to macOS as absent, and whitespace is blank.
    if(NOT "${CMAKE_MATCH_1}" MATCHES "[^ \t\r\n]")
        message(FATAL_ERROR "${usageKey} in ${BUNDLE_PLIST} has no text, which macOS treats as no key at all")
    endif()
endforeach()

message(STATUS "both macOS usage strings are present in ${BUNDLE_PLIST}")
