#!/bin/bash
###########################################################################
#   Copyright (C) 2023-2024  by Stephen Lyons - slysven@virginmedia.com   #
#                                                                         #
#   This program is free software; you can redistribute it and/or modify  #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 2 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
#   This program is distributed in the hope that it will be useful,       #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#   GNU General Public License for more details.                          #
#                                                                         #
#   You should have received a copy of the GNU General Public License     #
#   along with this program; if not, write to the                         #
#   Free Software Foundation, Inc.,                                       #
#   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
###########################################################################

# Version: 1.6.0    Changed to allow scripts to be used for Mudlet's own
#                   CI/CB process on AppVeyor.
#          1.5.0    Change BUILD_TYPE to BUILD_CONFIG to avoid clash with
#                   CI/CB system using same variable
#          1.4.0    No change
#          1.3.0    Remove used of the no longer supported/used by us QT5
#                   Gamepad stuff (since PR #6787 was merged into
#                   the development branch)
#          1.2.0    No changes
#          1.1.0    Updated to bail out if there isn't a mudlet.exe file to
#                   work with
#          1.0.0    Original version

# Script to each time to package all the files needed to run Mudlet on
# Windows in a directory which can be used both on this PC and others.
# For AppVeyor CI builds that lead to a "Release" or "PTB" build these need
# to be positioned in a sub-directory in another Git repository
# ("Squirrel.windows") that must be fetched otherwise (for both CI and other
# builds) zip everything up in an archive file (to be uploaded to Mudlet's
# own Website for the other CI builds that are NOT "Release" or "PTB" ones.

# To be used AFTER setup-windows-sdk.sh and build-mudlet-for-windows.sh
# have been run.

# Exit codes (some shared with other two scripts):
# 0 - Everything is fine. 8-)
# 1 - Failure to change to a directory
# 2 - Unsupported MSYS2/MINGGW shell type (only "MINGW32" or "MINGW64" currently supported)
# 3 - Unsupported build type (BUILD_CONFIG set, but not to "debug" or "release")
# 4 - Directory to be used to assemble the package is NOT empty
# 5 - Invalid configuration requested (32bit Qt6 builds are not possible)
# 7 - No mudlet.exe file found to work with (failed to compile in prior stage?)
# 9 - Failed to upload package to Mudlet website

if [ -z "${QT_MAJOR_VERSION}" ]; then
  # Assume previously used Qt5 unless told otherwise
  QT_MAJOR_VERSION="5"
  export QT_MAJOR_VERSION
  if [ -z "${APPVEYOR}" ]; then
    # Don't bother reporting this for CI builds as the previous script will do so
    echo "Assuming a build with Qt 5.x in absence of a QT_MAJOR_VERSION environmental variable."
  fi
fi

if [ -z "${BUILD_CONFIG}" ]; then
  # If this is present and set to "debug" then we'll do a debug type build but
  # otherwise we'll keep it as "release":
  BUILD_CONFIG="release"
  export BUILD_CONFIG
fi

if [ -z "${BUILDCOMPONENT}" ]; then
  if [ "${MSYSTEM}" = "MINGW64" ]; then
    # We are running in a 64-Bit terminal so assume that that is what the user
    # to build:
    BUILD_BITNESS="64"
    BUILDCOMPONENT="x86_64"
  elif [ "${MSYSTEM}" = "MINGW32" ]; then
    # We are running in a 32-Bit terminal so assume that that is what the user
    # to build (only possible to do a "base" build - using mingw32-qmake
    # directly as there is not a 32-Bit Qt Creator nowadays):
    BUILD_BITNESS="32"
    BUILDCOMPONENT="i686"
  elif [ "${MSYSTEM}" = "MSYS" ]; then
    echo "Please run this script from an MINGW32 or MINGW64 type bash terminal appropriate"
    echo "to the bitness you want to work on. You may do this once for each of them should"
    echo "you wish to do both."
    exit 2
  elif [ -z "${MSYSTEM}" ]; then
    echo "The environmental variable MSYSTEM is not set to anything so something is amiss"
    echo "Please rerun this in a Mingw-w64 MINGW32 or MINGW64 bash terminal."
    exit 2
  else
    echo "This script is not set up to handle systems of type ${MSYSTEM}, only MINGW32 or"
    echo "MINGW64 are currently supported. Please rerun this in a bash terminal of one"
    echo "of those two types."
    exit 2
  fi
  export BUILD_BITNESS
  export BUILDCOMPONENT
fi

if [ "${QT_MAJOR_VERSION}" = "6" ] && [ "${BUILD_BITNESS}" = "32" ]; then
    echo ""
    echo "Sorry, but it is not possible to perform 32-bit (i686) builds with Qt 6, build aborted!"
    exit 5
fi

if [ -z "${MINGW_INTERNAL_BASE_DIR}" ]; then
  # Variable not set so do so now - see setup-windows-sdk.sh why we are not
  # using the MINGW32/MINGW64 files that Appveyor might provide.
  MINGW_BASE_DIR="C:\msys64\mingw${BUILD_BITNESS}"
  export MINGW_BASE_DIR
  # Provide an equivalent POSIX format path for internal usage:
  MINGW_INTERNAL_BASE_DIR="$(cygpath -u "${MINGW_BASE_DIR}")"
  export MINGW_INTERNAL_BASE_DIR
fi

# Adjust path so directories we want are prepended if not present:
# From https://stackoverflow.com/a/48185201/4805858:
case :$PATH: in
  *:/usr/bin:*)
    ;; # do nothing, it's there
  *)
    PATH="/usr/bin:${PATH}"
    export PATH
    echo "Prepending /usr/bin to PATH"
    ;;
esac
case :$PATH: in
  *:${MINGW_INTERNAL_BASE_DIR}/bin:*)
    ;; # do nothing, it's there
  *)
    PATH="${MINGW_INTERNAL_BASE_DIR}/bin:${PATH}"
    export PATH
    echo "Prepending ${MINGW_INTERNAL_BASE_DIR}/bin to PATH"
    ;;
esac
case :$PATH: in
  *:${MINGW_INTERNAL_BASE_DIR}/usr/local/bin:*)
    ;; # do nothing, it's there
  *)
    PATH="${MINGW_INTERNAL_BASE_DIR}/usr/local/bin:${PATH}"
    export PATH
    echo "Prepending ${MINGW_INTERNAL_BASE_DIR}/usr/local/bin to PATH"
    ;;
esac

# We do not need LUA_PATH or LUA_CPATH when packaging things.

if [ -z "${BUILD_DIR}" ]; then
  # Variable not set so do so now
  if [ -z "${APPVEYOR_BUILD_FOLDER}" ]; then
    # The above will be defined for AppVeyor CI builds so this is not one of
    # those, and we need to allow for the end user to have multiple
    # builds in different directories or (for 64bit builds) to use either Qt 5 or 6:
    BUILD_DIR="${HOME}/src/mudlet/build-${MSYSTEM}-qt${QT_MAJOR_VERSION}"
  else
    # On CI builds we can use a plain build folder under the main /c/projects/mudlet
    # directory where the code is automagically placed for us:  
    BUILD_DIR="${APPVEYOR_BUILD_FOLDER}/build"
  fi
  BUILD_DIR="$(echo "${BUILD_DIR}" | sed -e 's|C:|/c|g' | sed -e 's|\\|/|g')"
  export BUILD_DIR
fi

# In practice this is where the Mudlet source code git repository is placed:
PARENT_OF_BUILD_DIR="$(echo "${BUILD_DIR}" | sed -e "s|/[^/]*$||")"
export PARENT_OF_BUILD_DIR

# Extract version information from qmake project file
# sed is used to remove the spaces either side of the `=` in the one line in
# the file that will match:
VERSION=$(grep "^VERSION = " "${PARENT_OF_BUILD_DIR}/src/mudlet.pro" | sed -e 's/VERSION = //g')
export VERSION

# Identify what we are going to do:
if [ -n "${APPVEYOR}" ]; then
  # This is an Appveyor CI build
  PACKAGE_DIR="${PARENT_OF_BUILD_DIR}/package"
  if [ "${APPVEYOR_REPO_NAME}" = "Mudlet/Mudlet" ]; then
    # This is being run on Mudlet's own repo
    if [ -n "${APPVEYOR_REPO_TAG_NAME}" ]; then
      # It is a build triggered by a tagged commit - so it is likely to be a
      # proper RELEASE build.
      TASK="RELEASE"
      # This will only persist into the QMake/CMake makefile generation process
      # if the project files for them has been edited to allow this through as
      # an empty string:
      MUDLET_VERSION_BUILD=""
    elif [ "${APPVEYOR_SCHEDULED_BUILD}" = "True" ]; then
      # It is a scheduled build so it is a Public Test Build
      TASK="PTB"
      BUILD_COMMIT=$(git rev-parse --short HEAD | sed 's/.*/\L&/g')
      MUDLET_VERSION_BUILD="-ptb-$(date -u -Idate)"
      export BUILD_COMMIT
    elif [ -n "${APPVEYOR_PULL_REQUEST_NUMBER}" ]; then
      # It is a PR buiild
      TASK="PR"
      # AppVeyor builds of PRs merge the PR head onto the current development
      # branch creating a new commit - as such we need to refer to the commit
      # Git SHA1 supplied to us rather than trying to back track to the
      # ancestor in the "working" tree (though it does mean the code state is
      # not accurately described as it only reports the PR's head without
      # reference to the state of the development at the time of the build:
      # MUDLET_VERSION_BUILD might be an empty string before this line or it
      # could be a hyphen prefixed string to identify a 3rd party build
      BUILD_COMMIT=$(git rev-parse --short "${APPVEYOR_PULL_REQUEST_HEAD_COMMIT}"| sed 's/.*/\L&/g')
      MUDLET_VERSION_BUILD=$(echo "${MUDLET_VERSION_BUILD}-testing-pr${APPVEYOR_PULL_REQUEST_NUMBER}" | sed 's/.*/\L&/g')
      if [ "${BUILD_CONFIG}" = "debug" ]; then
        ZIP_FILE_NAME="mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-windows-x${BUILD_BITNESS}-qt${QT_MAJOR_VERSION}-debug.zip"
      else
        ZIP_FILE_NAME="mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-windows-x${BUILD_BITNESS}-qt${QT_MAJOR_VERSION}.zip"
      fi
      export BUILD_COMMIT
      export ZIP_FILE_NAME
    else
      # It is a testing build which needs an archive to be made with a
      # specific name
      TASK="TESTING"
      BUILD_COMMIT=$(git rev-parse --short HEAD | sed 's/.*/\L&/g')
      if [ -n "${MUDLET_VERSION_BUILD}" ]; then
        MUDLET_VERSION_BUILD=$(echo "${MUDLET_VERSION_BUILD}-testing" | sed 's/.*/\L&/g')
      fi
      # MUDLET_VERSION_BUILD could be an empty string but it is intended for
      # third party packagers to tag customised versions of Mudlet:
      if [ "${BUILD_CONFIG}" = "debug" ]; then
        ZIP_FILE_NAME="mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-windows-x${BUILD_BITNESS}-qt${QT_MAJOR_VERSION}-debug.zip"
      else
        ZIP_FILE_NAME="mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-windows-x${BUILD_BITNESS}-qt${QT_MAJOR_VERSION}.zip"
      fi
      export BUILD_COMMIT
      export ZIP_FILE_NAME
    fi
  else
    # Not Mudlet's repository so just produce a zip file
    TASK="ZIP"
    BUILD_COMMIT=$(git rev-parse --short HEAD | sed 's/.*/\L&/g')
    # MUDLET_VERSION_BUILD could be an empty string but it is intended for
    # third party packagers to tag customised versions of Mudlet:
    if [ -n "${MUDLET_VERSION_BUILD}" ]; then
      MUDLET_VERSION_BUILD=$(echo "${MUDLET_VERSION_BUILD}" | sed 's/.*/\L&/g')
    fi
    if [ "${BUILD_CONFIG}" = "debug" ]; then
      ZIP_FILE_NAME="mudlet-${VERSION}${MUDLET_VERSION_BUILD}-x${BUILD_BITNESS}-qt${QT_MAJOR_VERSION}-${BUILD_COMMIT}-debug.zip"
    else
      ZIP_FILE_NAME="mudlet-${VERSION}${MUDLET_VERSION_BUILD}-x${BUILD_BITNESS}-qt${QT_MAJOR_VERSION}-${BUILD_COMMIT}.zip"
    fi
    export BUILD_COMMIT
    export ZIP_FILE_NAME
  fi
else
  # Not an appveyor CI build so just produce an archive
  PACKAGE_DIR="${PARENT_OF_BUILD_DIR}/package-${MSYSTEM}-qt${QT_MAJOR_VERSION}-${BUILD_CONFIG}"
  TASK="ZIP"
  BUILD_COMMIT=$(git rev-parse --short HEAD | sed 's/.*/\L&/g')
  # MUDLET_VERSION_BUILD could be an empty string but it is intended for
  # third party packagers to tag customised versions of Mudlet:
  if [ -n "${MUDLET_VERSION_BUILD}" ]; then
    MUDLET_VERSION_BUILD=$(echo "${MUDLET_VERSION_BUILD}" | sed 's/.*/\L&/g')
  fi
  if [ "${BUILD_CONFIG}" = "debug" ]; then
    ZIP_FILE_NAME="mudlet-${VERSION}${MUDLET_VERSION_BUILD}-x${BUILD_BITNESS}-qt${QT_MAJOR_VERSION}-${BUILD_COMMIT}-debug.zip"
  else
    ZIP_FILE_NAME="mudlet-${VERSION}${MUDLET_VERSION_BUILD}-x${BUILD_BITNESS}-qt${QT_MAJOR_VERSION}-${BUILD_COMMIT}.zip"
  fi
  export BUILD_COMMIT
  export ZIP_FILE_NAME
fi
export PACKAGE_DIR
export TASK
export MUDLET_VERSION_BUILD

### End of common(-ish) configuration for all three script files.

# Now display all the variables for checking in build logs:
echo ""
echo "Current variables:"
echo "APPVEYOR: ${APPVEYOR}"
if [ -n "${APPVEYOR}" ]; then
  echo "  APPVEYOR_BUILD_FOLDER: ${APPVEYOR_BUILD_FOLDER}"
  echo "  APPVEYOR_REPO_NAME: ${APPVEYOR_REPO_NAME}"
  echo "  APPVEYOR_REPO_TAG: ${APPVEYOR_REPO_TAG}"
  echo "  APPVEYOR_PULL_REQUEST_NUMBER: ${APPVEYOR_PULL_REQUEST_NUMBER}"
  echo "  APPVEYOR_REPO_COMMIT: ${APPVEYOR_REPO_COMMIT}"
  echo "  APPVEYOR_PULL_REQUEST_HEAD_COMMIT: ${APPVEYOR_PULL_REQUEST_HEAD_COMMIT}"
  echo "  APPVEYOR_PROJECT_SLUG: ${APPVEYOR_PROJECT_SLUG}"
fi
echo "BUILD_BITNESS: ${BUILD_BITNESS}"
echo "BUILD_DIR: ${BUILD_DIR}"
echo "BUILDCOMPONENT: ${BUILDCOMPONENT}"
echo "BUILD_CONFIG: ${BUILD_CONFIG}"
echo "MINGW_BASE_DIR: ${MINGW_BASE_DIR}"
echo "MINGW_INTERNAL_BASE_DIR: ${MINGW_INTERNAL_BASE_DIR}"
echo "MINGW_PREFIX: ${MINGW_PREFIX}"
echo "MSYSTEM: ${MSYSTEM}"
# If not an empty string this should begin with a '-' (hyphen)
echo "MUDLET_VERSION_BUILD: ${MUDLET_VERSION_BUILD}"
echo "PACKAGE_DIR: ${PACKAGE_DIR}"
echo "PATH: ${PATH}"
echo "PARENT_OF_BUILD_DIR: ${PARENT_OF_BUILD_DIR}"
echo "QT_MAJOR_VERSION: ${QT_MAJOR_VERSION}"
echo "TASK: ${TASK}"
echo "VERSION: ${VERSION}"
echo "ZIP_FILE_NAME=${ZIP_FILE_NAME}"
echo "HOME directory, (Windows form): $(cygpath -w "${HOME}")"
echo "HOME directory, (POSIX form): $(cygpath -u "${HOME}")"
echo ""

if [ -d "${PACKAGE_DIR}" ]; then
  # The wanted packaging dir exists - as is wanted
  echo "Checking for an empty ${PACKAGE_DIR} in which to assemble files..."
  echo ""
  if [ -n "$(ls -A "${PACKAGE_DIR}")" ]; then
    # but it isn't empty...
    echo "${PACKAGE_DIR} does not appear to be empty, please"
    echo "erase everything there and try again."
    exit 4
  fi
else
  echo "Creating ${PACKAGE_DIR} in which to assemble files..."
  echo ""
  # This will create the directory if it doesn't exist but won't moan if it does
  mkdir -p "${PACKAGE_DIR}"
fi
cd "${PACKAGE_DIR}" || exit 1
echo "Moving to ${PACKAGE_DIR}"
echo "Copying wanted compiled files from ${BUILD_DIR}-${BUILD_CONFIG}..."
echo ""

if [ ! -f "${BUILD_DIR}/${BUILD_CONFIG}/mudlet.exe" ]; then
  echo "ERROR: no Mudlet executable found - did the previous build"
  echo "complete sucessfully?"
  exit 7
fi

echo "Copying mudlet executable to packaging directory."
cp -v "${BUILD_DIR}/${BUILD_CONFIG}/mudlet.exe" "${PACKAGE_DIR}/"
if [ -f "${BUILD_DIR}/${BUILD_CONFIG}/mudlet.exe.debug" ]; then
  # This will only exist for debug builds (with a separate debug information
  # file), which IS what we asked for during compilation as it makes the
  # executable smaller and quicker to load:
  echo "Copying mudlet debug to packaging directory."
  cp -v "${BUILD_DIR}/${BUILD_CONFIG}/mudlet.exe.debug" "${PACKAGE_DIR}/"
fi

echo "Running windeployqt..."
if [ "${QT_MAJOR_VERSION}" = "6" ]; then
  # We could include the virtual keyboard - but that can be changed if it is
  # deemed to be desireable - also the --debug / --release flags don't work/
  # or are not needed for Qt6 on Mingw32/Mingw64 as the debug information is
  # shipped separatly rather than being included in the Qt6 dll files:
  "${MINGW_INTERNAL_BASE_DIR}/bin/windeployqt-qt6.exe" --no-virtualkeyboard --ignore-library-errors ./${EXECUTABLE_NAME}
else
  # Since Qt 5.14 using the --release switch is broken (it now seems to be
  # assumed), --debug still seems to work - sort of - it doesn't copy the
  # MINGW .debug files for the Qt libraries. As MSYS2+Mingw64 offers Qt 5.15+
  # this isn't an issue for us. https://bugreports.qt.io/browse/QTBUG-80806 was
  # relevant.
# Actually it seems that the debug switch might not be needed for Qt 5.15 either
# given that Mingw-w64 ships the debug information for the Qt libraries
# separately
#  if [ "${BUILD_CONFIG}" = "debug" ]; then
#    "${MINGW_INTERNAL_BASE_DIR}/bin/windeployqt.exe" --debug --no-virtualkeyboard ./${EXECUTABLE_NAME}
#  else
    "${MINGW_INTERNAL_BASE_DIR}/bin/windeployqt.exe" --no-virtualkeyboard ./${EXECUTABLE_NAME}
#  fi
fi
echo "   ... completed windeployqt."
echo ""

# Stupidly windeployqt does not copy the .debug files that actually contains
# the debug information for the Qt library files - so copy them manually:
# They have been deduced by looking for matching 'Xxxx(.dll).debug' files for
# each 'Xxxx.dll' one so A) they may not be complete and B) are only for
# the Qt libraries - other third party ones are not necessarily covered:
function copyDebugFiles () {
  local SOURCE_DIR="${1}"
  local DESTINATION_DIR="${2}"
  while [ -n "${3}" ]; do
    local LIBNAME="${3}"
    if [ "${QT_MAJOR_VERSION}" = "5" ]; then
      # Qt5 has .debug appended after a retained .dll extension whereas
      # Qt6 puts it directly after the filename without ".dll": 
      cp -v -p "${SOURCE_DIR}/${LIBNAME}.dll.debug" "${DESTINATION_DIR}"
    else
      cp -v -p "${SOURCE_DIR}/${LIBNAME}.debug" "${DESTINATION_DIR}"
    fi
    shift
  done
}

if [ "${BUILD_CONFIG}" = "debug" ]; then
  echo "Copying debug libraries..."
  if [ "${QT_MAJOR_VERSION}" = "6" ]; then
    # Nothing in here for MINWG32 case as Qt6 DOES NOT SUPPORT 32-Bits and it
    # will already have been aborted.

    # The core library files are located in a bin directory rather than a Qt
    # share one:
    #              Source                                                   Destination             Debug files
    copyDebugFiles "${MINGW_BASE_DIR}/bin"                                  "."                     "Qt6Core" "Qt6Core5Compat" "Qt6Gui" "Qt6Multimedia" "Qt6Network" "Qt6OpenGL" "Qt6OpenGLWidgets" "Qt6Svg" "Qt6Widgets" "Qt6TextToSpeech" "Qt6UiTools"
    copyDebugFiles "${MINGW_BASE_DIR}/share/qt6/plugins/generic"            "./generic/"            "qtuiotouchplugin"
    copyDebugFiles "${MINGW_BASE_DIR}/share/qt6/plugins/iconengines"        "./iconengines/"        "qsvgicon"
    copyDebugFiles "${MINGW_BASE_DIR}/share/qt6/plugins/imageformats"       "./imageformats/"       "qgif" "qicns" "qico" "qjp2" "qjpeg" "qmng" "qsvg" "qtga" "qtiff" "qwbmp" "qwebp"
    copyDebugFiles "${MINGW_BASE_DIR}/share/qt6/plugins/multimedia"         "./multimedia/"         "ffmpegmediaplugin"
    copyDebugFiles "${MINGW_BASE_DIR}/share/qt6/plugins/networkinformation" "./networkinformation/" "qglib" "qnetworklistmanager"
    copyDebugFiles "${MINGW_BASE_DIR}/share/qt6/plugins/platforms"          "./platforms/"          "qwindows"
    copyDebugFiles "${MINGW_BASE_DIR}/share/qt6/plugins/styles"             "./styles/"             "qmodernwindowsstyle"
    copyDebugFiles "${MINGW_BASE_DIR}/share/qt6/plugins/texttospeech"       "./texttospeech/"       "qtexttospeech_mock" "qtexttospeech_sapi"
    copyDebugFiles "${MINGW_BASE_DIR}/share/qt6/plugins/tls"                "./tls/"                "qcertonlybackend" "qopensslbackend" "qschannelbackend"
  else
    #              Source                                                Destination          Debug files
    copyDebugFiles "${MINGW_BASE_DIR}/bin"                               "."                  "Qt5Core" "Qt5Gui" "Qt5Multimedia" "Qt5Network" "Qt5Svg" "Qt5Widgets" "Qt5TextToSpeech"
    copyDebugFiles "${MINGW_BASE_DIR}/share/qt5/plugins/audio"           "./audio/"           "qtaudio_windows"
    copyDebugFiles "${MINGW_BASE_DIR}/share/qt5/plugins/bearer"          "./bearer/"          "qgenericbearer"
    copyDebugFiles "${MINGW_BASE_DIR}/share/qt5/plugins/iconengines"     "./iconengines/"     "qsvgicon"
    if [ "${MSYSTEM}" = "MINGW64" ]; then
      copyDebugFiles "${MINGW_BASE_DIR}/share/qt5/plugins/imageformats"  "./imageformats/"    "qgif" "qicns" "qico" "qjp2" "qjpeg" "qmng" "qsvg" "qtga" "qtiff" "qwbmp" "qwebp"
    else
      # The library for the "multiple network graphics" file format is not
      # included in the MINGW32 environment!
      copyDebugFiles "${MINGW_BASE_DIR}/share/qt5/plugins/imageformats"  "./imageformats/"    "qgif" "qicns" "qico" "qjp2" "qjpeg" "qsvg" "qtga" "qtiff" "qwbmp" "qwebp"
    fi
    copyDebugFiles "${MINGW_BASE_DIR}/share/qt5/plugins/mediaservice"    "./mediaservice/"    "dsengine" "qtmedia_audioengine" "wmfengine"
    copyDebugFiles "${MINGW_BASE_DIR}/share/qt5/plugins/platforms"       "./platforms/"       "qwindows"
    copyDebugFiles "${MINGW_BASE_DIR}/share/qt5/plugins/playlistformats" "./playlistformats/" "qtmultimedia_m3u"
    copyDebugFiles "${MINGW_BASE_DIR}/share/qt5/plugins/styles"          "./styles/"          "qwindowsvistastyle"
    copyDebugFiles "${MINGW_BASE_DIR}/share/qt5/plugins/texttospeech"    "./texttospeech/"    "qtexttospeech_sapi"
  fi
  echo "    ... done copying debug files."
  echo ""
fi

# Non-system libraries (both Qt and others) found in executable: 

# For ones used by lua modules this can manifest as being unable to "require"
# the library within lua and doing the above "ntldd" check revealed that, for
# instance, "luasql/sqlite3.dll" needed "libsqlite3-0.dll"!

echo "Examining Mudlet application to identify other needed libraries..."
# The greps filter out only paths that:
# * do not contain "Qt5" or "Qt6" 
# * include "mingw" (to capture those with "mingw32" or "mingw64")
# * include "bin" for the path where Mingw32/64 keep their main library files
# The cuts ensures we only get the file and path to the library after the =>
# in the lines that match:
NEEDED_LIBS=( $("${MINGW_INTERNAL_BASE_DIR}/bin/ntldd" --recursive ./${EXECUTABLE_NAME} \
  | /usr/bin/grep -v "Qt[56]" \
  | /usr/bin/grep -i "mingw" \
  | /usr/bin/grep -i "bin" \
  | /usr/bin/cut -d ">" -f2 \
  | /usr/bin/cut -d "(" -f1 \
  | /usr/bin/sed -e 's|C:|/c|g' \
  | /usr/bin/sed -e 's|\\|/|g' \
  | /usr/bin/sort) )
echo ""
echo "Copying identified libraries..."
for LIB in "${NEEDED_LIBS[@]}" ; do
  cp -v -p "${LIB}" . ;
done
echo "    ... done copying identified."

echo ""
echo "Copying other, known to be needed, libraries in..."
# libjasper to libwebpdemux-2 are additional image format handlers that Qt can
# use if they are present.
# libsqlite3 and libyajl are needed by lua modules (luasql-sqlite3) and at Mudlet run time.
cp -v -p -t . \
    "${MINGW_INTERNAL_BASE_DIR}/bin/libjasper.dll" \
    "${MINGW_INTERNAL_BASE_DIR}/bin/libjpeg-8.dll" \
    "${MINGW_INTERNAL_BASE_DIR}/bin/libtiff-6.dll" \
    "${MINGW_INTERNAL_BASE_DIR}/bin/libwebp-7.dll" \
    "${MINGW_INTERNAL_BASE_DIR}/bin/libwebpdemux-2.dll" \
    "${MINGW_INTERNAL_BASE_DIR}/bin/libsqlite3-0.dll" \
    "${MINGW_INTERNAL_BASE_DIR}/bin/libyajl.dll"

echo ""
echo "Copying OpenSSL libraries in..."
# The openSSL libraries has a different name depending on the bitness:
if [ "${MSYSTEM}" = "MINGW32" ]; then
    cp -v -p -t . \
        "${MINGW_INTERNAL_BASE_DIR}/bin/libcrypto-3.dll" \
        "${MINGW_INTERNAL_BASE_DIR}/bin/libssl-3.dll"

elif [ "${MSYSTEM}" = "MINGW64" ]; then
    cp -v -p -t . \
        "${MINGW_INTERNAL_BASE_DIR}/bin/libcrypto-3-x64.dll" \
        "${MINGW_INTERNAL_BASE_DIR}/bin/libssl-3-x64.dll"

fi

echo ""
echo "Copying discord-rpc library in..."
if [ "${MSYSTEM}" = "MINGW32" ]; then
    cp -v -p "${PARENT_OF_BUILD_DIR}/3rdparty/discord/rpc/lib/discord-rpc32.dll"  .
elif [ "${MSYSTEM}" = "MINGW64" ]; then
    cp -v -p "${PARENT_OF_BUILD_DIR}/3rdparty/discord/rpc/lib/discord-rpc64.dll"  .
fi
echo ""

# Lua libraries:
# If there is a demand for other rocks in the Windows installer because of
# revisions to the mappers or geyser framework or popular demand otherwise then
# the rock for those will also have to be installed and their C(.dll)/Lua (.lua)
# files included here:
echo "Copying lua C libraries in..."
cp -v -p -t . \
    "${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/lfs.dll" \
    "${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/lpeg.dll" \
    "${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/lsqlite3.dll" \
    "${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/lua-utf8.dll" \
    "${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/rex_pcre.dll" \
    "${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/yajl.dll"

mkdir ./luasql
cp -v -p "${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/luasql/sqlite3.dll" ./luasql/sqlite3.dll
mkdir ./brimworks
cp -v -p "${MINGW_INTERNAL_BASE_DIR}/lib/lua/5.1/brimworks/zip.dll" ./brimworks/zip.dll
echo ""

echo "Copying Mudlet & Geyser Lua files and the Generic Mapper in..."
echo "Current directory is: $(pwd)"

# Using the '/./' notation provides the point at which rsync reproduces the
# directory structure from the source into the target and avoids the need
# to change directory before and after the rsync call:

# As written it copies every file but it should be polished up to skip unneeded
# ones:
rsync -avR "${PARENT_OF_BUILD_DIR}"/src/mudlet-lua/./* ./mudlet-lua/
echo ""

echo "Copying Lua code formatter Lua files in..."
# As written it copies every file but it should be polished up to skip unneeded
# ones:
rsync -avR "${PARENT_OF_BUILD_DIR}"/3rdparty/lcf/./* ./lcf/
echo ""

# Note we do NOT need the mudlet_??_??.qm files for the Mudlet application
# as we ship those inside the mudlet executable

echo "Copying Lua translation files in..."
rsync -avR --mkpath  "${PARENT_OF_BUILD_DIR}"/translations/lua/translated/./mudlet-lua_??_??.json ./translations/lua/translated/
echo ""

echo "Copying Hunspell dictionaries in..."
rsync -avR "${PARENT_OF_BUILD_DIR}"/src/./*.aff "${PARENT_OF_BUILD_DIR}"/src/./*.dic .

echo ""

# FINAL_DIR=$(/usr/bin/cygpath --windows "${PACKAGE_DIR}")
# echo "${FINAL_DIR} should contain everything needed to run Mudlet!"
# echo ""

if [ "${TASK}" = "ZIP" ]; then
  # Produce an archive file
  echo "Zipping everything up into an archive that can be moved to another machine:"
  /usr/bin/zip -rv9 "${ZIP_FILE_NAME}" ./*
  echo ""
  echo "   ... package-mudlet-for-windows.sh shell script finished."
  echo ""
  echo "   You may now run the ${EXECUTABLE_NAME} file in ${FINAL_DIR} or take the file"
  echo "   there: ${ZIP_FILE_NAME}.zip to somewhere else - even a different PC - unzip"
  echo "   everything and run the ${EXECUTABLE_NAME} file extracted from it..."
  cd ~ || exit 1
elif [ "${TASK}" = "PR" ] || [ "${TASK}" = "TESTING" ]; then
  # Produce an archive file with a specific name and upload it to Mudlet's own website
  echo "Zipping everything up into an archive that to be uploaded to Mudlet's website:"
  /usr/bin/zip -rv9 "${ZIP_FILE_NAME}" ./*

  # Temporarily, save file in Appveyor
  appveyor PushArtifact "$(/usr/bin/cygpath --windows "/c/projects/mudlet/package/${ZIP_FILE_NAME}")" -FileName "${ZIP_FILE_NAME}"

  echo ""
  echo "   Uploading archive file to Mudlet's website..."
  curl --fail --fail-early -i -T "${ZIP_FILE_NAME}" "https://make.mudlet.org/snapshots/${ZIP_FILE_NAME}"
  CURL_STATUS=$?
  if [ "${CURL_STATUS}" != "0" ]; then
    echo "   ... uploading failed, curl error code was: ${CURL_STATUS}."
    exit 9
  fi    
  echo "   ... upload completed."
  echo ""
  cd ~ || exit 1
else
  echo " TODO: nuget + squirrel.windows stuff not done yet!"
  exit 10
fi
echo "... package-mudlet-for-windows.sh shell script finished."
