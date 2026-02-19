#!/bin/bash
###########################################################################
#   Copyright (C) 2024-2024  by John McKisson - john.mckisson@gmail.com   #
#   Copyright (C) 2023-2025  by Stephen Lyons - slysven@virginmedia.com   #
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

set -x

# Version: 2.3.0    Add build counter suffix for multiple builds from same commit
#          2.2.0    Skip commit date check when build is manually forced
#          2.1.0    Remove MINGW32 since upstream no longer supports it
#          2.0.0    Rework to build on an MSYS2 MINGW64 Github workflow

# Exit codes:
# 0 - Everything is fine. 8-)
# 1 - Failure to change to a directory
# 2 - Unsupported MSYS2/MINGGW shell type
# 5 - Invalid configuration
# 9 - squirrel error

# Check if we're building from the Mudlet/Mudlet repository and not a fork
if [[ "${GITHUB_REPO_NAME}" != "Mudlet/Mudlet" ]]; then
  echo "This is not the main Mudlet repository - aborting!"
  exit 5
fi

if [ "${MSYSTEM}" = "MSYS" ]; then
  echo "Please run this script from a MINGW64 type bash terminal as the MSYS one"
  echo "does not supported what is needed."
  exit 2
elif [ "${MSYSTEM}" != "MINGW64" ]; then
  echo "This script is not set up to handle systems of type ${MSYSTEM}, only"
  echo "MINGW64 is currently supported. Please rerun this in a bash terminal of"
  echo "that type."
  exit 2
fi

if [ -z "${BUILD_ACTION}" ]; then
  # Error out if not set:
  echo "BUILD_ACTION not set in environment, check preceding steps!"
  exit 10
elif [ "${BUILD_ACTION}" == "Unknown" ] || [ "${BUILD_ACTION}" == "Archive" ]; then
  # Don't proceed further in this step - but abort in a "successful" manner
  echo "BUILD_ACTION being \"${BUILD_ACTION}\" means no deployment so exiting this step in the workflow immediately!"
  exit 0
fi

if [ -z "${VERSION}" ]; then
  # Error out if not set:
  echo "VERSION not set in environment, check preceding steps!"
  exit 10
fi

if [ "${BUILD_ACTION}" != "Release" ]; then
  if [ -z "${BUILD_COMMIT}" ]; then
    # Error out if not set:
    echo "BUILD_COMMIT not set in environment and this isn't a \"Release\" build, check preceding steps!"
    exit 10
  fi
  if [ -z "${MUDLET_VERSION_BUILD}" ]; then
    echo "MUDLET_VERSION_BUILD not set in environment and this isn't a \"Release\" build, check preceding steps!"
    exit 10
  fi
fi

cd "${GITHUB_WORKSPACE}" || exit 1

# Add nuget location to PATH
PATH="/c/ProgramData/Chocolatey/bin:${PATH}"
export PATH

# Check if GITHUB_REPO_TAG is "false"
if [[ "${GITHUB_REPO_TAG}" == "false" ]]; then
  echo "=== GITHUB_REPO_TAG is FALSE ==="

  # Check if this is a scheduled build
  if [[ "${GITHUB_SCHEDULED_BUILD}" == "true" ]]; then
    echo "=== GITHUB_SCHEDULED_BUILD is TRUE, this is a PTB ==="
    MUDLET_VERSION_BUILD="-ptb"
  else
    MUDLET_VERSION_BUILD="-testing"
    echo "=== GITHUB_SCHEDULED_BUILD is FALSE, this is NOT a PTB ==="
  fi

  # Check if this is a pull request
  if [[ -n "${GITHUB_PULL_REQUEST_NUMBER}" ]]; then
    # Use the specific commit SHA from the pull request head, since GitHub Actions merges the PR
    BUILD_COMMIT=$(git rev-parse --short "${GITHUB_PULL_REQUEST_HEAD_SHA}")
    MUDLET_VERSION_BUILD="${MUDLET_VERSION_BUILD}-PR${GITHUB_PULL_REQUEST_NUMBER}"
  else
    BUILD_COMMIT=$(git rev-parse --short HEAD)

    if [[ "${MUDLET_VERSION_BUILD}" == "-ptb" ]]; then
      # Get current date in YYYY-MM-DD format
      CURRENT_DATE=$(date +%F)
      MUDLET_VERSION_BUILD="${MUDLET_VERSION_BUILD}-${CURRENT_DATE}"
    fi
  fi
else
  echo "=== GITHUB_REPO_TAG is TRUE ==="
fi

# Convert to lowercase, not all systems deal with uppercase ASCII characters
# This will still be empty for a Release build
export MUDLET_VERSION_BUILD="${MUDLET_VERSION_BUILD,,}"
export BUILD_COMMIT="${BUILD_COMMIT,,}"

# Extract version from CMakeLists.txt
VERSION_LINE=$(grep "set(APP_VERSION" "${GITHUB_WORKSPACE}/CMakeLists.txt")
VERSION_REGEX='set\(APP_VERSION (.+)\)'

# Use Bash regex matching to extract version - don't double-quote these as that
# can mess things up!
if [[ ${VERSION_LINE} =~ ${VERSION_REGEX} ]]; then
  VERSION="${BASH_REMATCH[1]}"
fi

# For PTB builds, check if we need a build counter suffix
# This allows multiple builds from the same commit
BUILD_COUNTER_SUFFIX=""
if [[ "${MUDLET_VERSION_BUILD}" == -ptb* ]] && [[ -n "${BUILD_COMMIT}" ]]; then
  # Query the dblsqd feed for existing versions with this commit
  EXISTING_VERSIONS=$(curl --silent "https://feeds.dblsqd.com/MKMMR7HNSP65PquQQbiDIw/public-test-build/win/x86_64" | \
    jq --raw-output ".releases[].version" | \
    grep -E "${BUILD_COMMIT}(rebuild[0-9]+)?$" || true)

  if [[ -n "${EXISTING_VERSIONS}" ]]; then
    # Count existing versions and find the highest build number
    HIGHEST_BUILD=1
    while IFS= read -r ver; do
      if [[ "${ver}" =~ rebuild([0-9]+)$ ]]; then
        NUM="${BASH_REMATCH[1]}"
        if [[ "${NUM}" -gt "${HIGHEST_BUILD}" ]]; then
          HIGHEST_BUILD="${NUM}"
        fi
      fi
    done <<< "${EXISTING_VERSIONS}"

    # Next build number
    NEXT_BUILD=$((HIGHEST_BUILD + 1))
    BUILD_COUNTER_SUFFIX="rebuild${NEXT_BUILD}"
    echo "=== Found existing PTB builds for commit ${BUILD_COMMIT}, using build counter: ${NEXT_BUILD} ==="
  fi
fi

# Check if MUDLET_VERSION_BUILD is empty and print accordingly
if [[ -z "${MUDLET_VERSION_BUILD}" ]]; then
  # Probably a release build - so typical output could be:
  #    "BUILDING MUDLET 4.19.1
  echo "BUILDING MUDLET ${VERSION}"
else
  # Include Git SHA1 in the display of the build information
  # Probably a PTB - so typical output could be:
  #    "BUILDING MUDLET 4.19.1-ptb-2025-01-01-012345678
  # Or with build counter: "BUILDING MUDLET 4.19.1-ptb-2025-01-01-012345678rebuild2
  echo "BUILDING MUDLET ${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}${BUILD_COUNTER_SUFFIX}"
fi
if [ "${MAKE_PORTABLE}" == 'true' ]; then
  echo "Also producing a portable version of the above."
fi

PACKAGE_PATH="$(cygpath -au "${GITHUB_WORKSPACE}/package-${MSYSTEM}-${BUILD_CONFIG}")"
PACKAGE_WINPATH="$(cygpath -aw "${PACKAGE_PATH}")"
cd "${PACKAGE_PATH}" || exit 1

if [ "${BUILD_ACTION}" == "PullRequest" ] || [ "${BUILD_ACTION}" == "Testing" ]; then
  echo "=== Creating a snapshot build ==="
  PACKAGE_EXE="Mudlet.exe"
  echo "Renaming mudlet.exe to ${PACKAGE_EXE}"
  mv "${PACKAGE_PATH}/mudlet.exe" "${PACKAGE_PATH}/${PACKAGE_EXE}"

  if [ "${MAKE_PORTABLE}" == 'true' ]; then
    # Although the "portable" version is also a compressed archive it is a self
    # extracting executable one - which we have to make ourselves and include
    # the "portable.txt" sentinel file. By putting that file into
    # ${PACKAGE_PATH} and creating the self-extracting archive file then
    # removing the sentinel we do not have to copy all the files in
    # ${PACKAGE_PATH} to another location to make the portable one. Note that
    # the creation of the original (zip) artifact is done by the GH action using
    # the files placed in ${PACKAGE_PATH} as a separate workflow step.
    echo "=== Preparing Portable artifact for upload to make.mudlet.org ==="

    # Create portable.txt file to enable portable mode (empty file) - meaning
    # store the game data and settings under the directory where the executable
    # is located when it is extracted:
    touch "${PACKAGE_PATH}/portable.txt"
    echo "Created empty portable.txt file: ${PACKAGE_PATH}/portable.txt"

    # This name is more specific than originally coded but that was only
    # targeting "Release" builds
    if [ "${BUILD_ACTION}" == "PullRequest" ]; then
      PORTABLE_ARTIFACT_NAME="Mudlet-${VERSION}-pr${GITHUB_PULL_REQUEST_NUMBER}-${BUILD_COMMIT}-windows-64-portable.exe"
    else
      PORTABLE_ARTIFACT_NAME="Mudlet-${VERSION}-testing-${BUILD_COMMIT}-windows-64-portable.exe"
    fi
    PORTABLE_ARTIFACT_WINPATHORFILE="$(cygpath -aw "${GITHUB_WORKSPACE}/${PORTABLE_ARTIFACT_NAME}")"

    echo "Creating self-extracting archive from directory: $(basename "${PACKAGE_PATH}")"
    # Actually create the portable ZIP archive - put it in the root of our
    # working area:
    7z a -mx9 -bt -sfx "$(cygpath -au "${PORTABLE_ARTIFACT_WINPATHORFILE}")"

    # Make the detail available to the workflow file so it can be passed to the
    # upload action
    {
      echo "PORTABLE_ARTIFACT_NAME=${PORTABLE_ARTIFACT_NAME}"
      echo "PORTABLE_ARTIFACT_WINPATHORFILE=${PORTABLE_ARTIFACT_WINPATHORFILE}"
    } >> "${GITHUB_ENV}"

    echo ""
    echo "Created portable self-extracting archive: ${PORTABLE_FILENAME}"

    # Remove sentinel file:
    rm "${PACKAGE_PATH}/portable.txt"
    echo ""
    echo "Removed portable.txt sentinel file"
  fi

  # Define the upload filename - MUDLET_VERSION_BUILD will at least be something
  # like "-testing" or "-pr####" but NOT "-ptb-*" {PR commits did previously
  # start with "-testing-pr####" but that was confusing and has been changed}
  # THIS IS THE NAME GIVEN TO THE GHA "artifact" which is automagically made
  # as a zip archive file.
  ARTIFACT_NAME="Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-windows-64"
  # As a path this causes everything there to be zipped up into an archive which
  # is subsequently uploaded to our website as is (and NOT unzipped)
  ARTIFACT_WINPATHORFILE="$(cygpath -aw "${PACKAGE_PATH}")"
  # Append these variables to the GITHUB_ENV to make them available in
  # subsequent steps, the fourth one being 0 means "don't unzip the archive when
  # it is uploaded to the Mudlet website". In this place and further down when
  # appending to the GH Actions environment DO NOT add escaped double-quotes
  # around the string after the '=' such extra double quotes break things!
  {
    echo "ARTIFACT_NAME=${ARTIFACT_NAME}"
    echo "ARTIFACT_WINPATHORFILE=${ARTIFACT_WINPATHORFILE}"
    echo "ARTIFACT_COMPRESSION=9"
    echo "ARTIFACT_UNZIP=0"
  } >> "${GITHUB_ENV}"

else
  # A Public Test Build or a Release
  # Check if it's a Public Test Build
  if [[ "${GITHUB_SCHEDULED_BUILD}" == "true" ]]; then

    # Skip commit check if this is a manually forced build
    if [[ "${GITHUB_FORCE_BUILD}" == "true" ]]; then
      echo "=== Forced build requested, skipping commit date check ==="
    else
      # Get the commit date of the last commit
      COMMIT_DATE=$(git show -s --format="%cs")
      # Get yesterday's date in the same format
      YESTERDAY_DATE=$(date --date="yesterday" +%Y-%m-%d)

      if [[ "${COMMIT_DATE}" < "${YESTERDAY_DATE}" ]]; then
        echo "=== No new commits, aborting public test build generation ==="
        exit 0
      else
        echo "=== New commits, continuing to create a public test build ==="
      fi
    fi

  # Set parameters for Clowd.Squirrel and other stages
  if [ "${BUILD_ACTION}" == "PublicTest" ]; then
    echo "=== Creating a Public Test build ==="
    # Squirrel uses the name of the binary for the Start menu, so need to rename
    # it so it doesn't get mixed up with the Release one:
    PACKAGE_EXE="Mudlet PTB.exe"
    INTERMEDIATE_ARTIFACT_NAME="Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-windows-64"
    if [ "${MAKE_PORTABLE}" == 'true' ]; then
      PORTABLE_ARTIFACT_NAME="Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-windows-64-portable.exe"
    fi
    # Allow public test builds to be installed side by side with the release
    # builds by renaming the app as well
    # No dots in the <id>: according to the guidelines by Squirrel
    NAME_SUFFIX='_64_-PublicTestBuild'
    INSTALLER_ICON_WINFILE=$(cygpath -aw "${GITHUB_WORKSPACE}/src/icons/mudlet_ptb.ico")
    ID='Mudlet_64_-PublicTestBuild'
    TITLE='Mudlet x64 (Public Test Build)'
    LOADING_GIF="$(cygpath -aw "${GITHUB_WORKSPACE}/installers/windows/splash-installing-ptb-2x.png")"
    INSTALLER_VERSION="${VERSION}-ptb-${BUILD_COMMIT,,}"
    # The name we want to use for the installer;
    # Typically of form: 'Mudlet-4.19.1-ptb-2025-01-01-012345678-windows-64.exe'
    INSTALLER_EXE="Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-windows-64.exe"
    DBLSQD_VERSION_STRING="${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT,,}"
    # The name that has to be passed as the artifact so that the Mudlet website
    # will accept it as a PTB:
    ARTIFACT_NAME="Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-windows-64-installer.exe"
  else
    echo "=== Creating a release build ==="
    PACKAGE_EXE="Mudlet.exe"
    INTERMEDIATE_ARTIFACT_NAME="Mudlet-${VERSION}-windows-64"
    if [ "${MAKE_PORTABLE}" == 'true' ]; then
      PORTABLE_ARTIFACT_NAME="Mudlet-${VERSION}-windows-64-portable.exe"
    fi
    NAME_SUFFIX='_64_'
    INSTALLER_ICON_WINFILE=$(cygpath -aw "${GITHUB_WORKSPACE}/src/icons/mudlet.ico")
    ID='Mudlet_64_'
    TITLE='Mudlet x64'
    LOADING_GIF=$(cygpath -aw "${GITHUB_WORKSPACE}/installers/windows/splash-installing-2x.png")
    # Typically       '4.19.1'
    INSTALLER_VERSION="${VERSION}"
    # Typically of form: 'Mudlet-4.19.1-windows-64-installer.exe'
    INSTALLER_EXE="Mudlet-${VERSION}-windows-64-installer.exe"
    DBLSQD_VERSION_STRING="${VERSION}"
  fi

  echo "Renaming mudlet.exe to ${PACKAGE_EXE}"
  mv "${PACKAGE_PATH}/mudlet.exe" "${PACKAGE_PATH}/${PACKAGE_EXE}"

  # Create squirrel sidecar file to mark only the main exe for Start Menu shortcut
  # This prevents crashpad_handler.exe from getting its own Start Menu entry
  echo "1" > "${PACKAGE_PATH}/${PACKAGE_EXE}.squirrel"

  PACKAGE_EXE_PATHFILE="$(cygpath -au "${PACKAGE_PATH}/${PACKAGE_EXE}")"
  PACKAGE_EXE_WINPATHFILE="$(cygpath -aw "${PACKAGE_EXE_PATHFILE}")"

  echo "=== Cloning installer project ==="
  git clone https://github.com/Mudlet/installers.git "${GITHUB_WORKSPACE}/installers"
  cd "${GITHUB_WORKSPACE}/installers/windows" || exit 1

  echo "=== Preparing an intermediate artifact of the code ==="
  # What will it be called:
  if [[ -z "${MUDLET_VERSION_BUILD}" ]]; then
    INTERMEDIATE_ARTIFACT_NAME="Mudlet-${VERSION}-windows-64"
  else
    INTERMEDIATE_ARTIFACT_NAME="Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}${BUILD_COUNTER_SUFFIX}-windows-64"
  fi

  echo "=== Preparing an intermediate artifact of the (signed) code ==="
  # This intermediate will NOT be uploaded but will remain on the GH server as
  # an artifact for a default (90?) days
  INTERMEDIATE_ARTIFACT_WINPATH="$(cygpath -aw "${PACKAGE_PATH}")"
  {
    echo "INTERMEDIATE_ARTIFACT_NAME=${INTERMEDIATE_ARTIFACT_NAME}"
    echo "INTERMEDIATE_ARTIFACT_WINPATH=${INTERMEDIATE_ARTIFACT_WINPATH}"
  } >> "${GITHUB_ENV}"

  echo "=== Installing Clowd.Squirrel for Windows ==="
  # Although archived this is a replacement for the squirrel.windows original
  nuget install Clowd.Squirrel -ExcludeVersion -NonInteractive

  echo "=== Setting up directories ==="
  RELEASE_DIR="$(cygpath -au "${GITHUB_WORKSPACE}/release")"
  mkdir -p "${RELEASE_DIR}"
  RELEASE_WINDIR="$(cygpath -aw "${RELEASE_DIR}")"

  echo "=== Preparing to create installer ==="
  # Set parameters for Clowd.Squirrel and other stages
  if [[ "${GITHUB_SCHEDULED_BUILD}" == "true" ]]; then
    # Allow public test builds to be installed side by side with the release
    # builds by renaming the app
    # No dots in the <id>: Guidelines by Squirrel
    NAME_SUFFIX='_64_-PublicTestBuild'
    INSTALLER_ICON_WINFILE=$(cygpath -aw "${GITHUB_WORKSPACE}/src/icons/mudlet_ptb.ico")
    ID='Mudlet_64_-PublicTestBuild'
    TITLE='Mudlet (Public Test Build)'
    LOADING_GIF="$(cygpath -aw "${GITHUB_WORKSPACE}/installers/windows/splash-installing-ptb-2x.png")"
    # Because the packaging tools use "Semantic Versioning" it makes sense
    # use the date in a number year-first form rather than the SHA1 as
    # that enables chonological ordering - although we do not seem to rely on it
    # https://learn.microsoft.com/en-us/nuget/concepts/package-versioning?tabs=semver20sort
    # This suggested that "4.19.1-ptb.20250811" would work and be sorted.
    # However it is rejected as invalid. This would seem to suggest that it is
    # using the older:
    # https://learn.microsoft.com/en-us/nuget/concepts/package-versioning?tabs=semver10sort
    # which cannot handle dotted numbers. So revert to original methodology that
    # appended the short commit SHA1 - and just not worry about any sort of
    # sorting:
    INSTALLER_VERSION="${VERSION}-ptb-${BUILD_COMMIT,,}${BUILD_COUNTER_SUFFIX}"
    # The name we want to use for the installer;
    # Typically of form: 'Mudlet-4.19.1-ptb-2025-01-01-012345678-windows-64.exe'
    # Or with build counter: 'Mudlet-4.19.1-ptb-2025-01-01-012345678rebuild2-windows-64.exe'
    INSTALLER_EXE="Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}${BUILD_COUNTER_SUFFIX}-windows-64.exe"
    DBLSQD_VERSION_STRING="${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT,,}${BUILD_COUNTER_SUFFIX}"
    # The name that has to be passed as the artifact so that the Mudlet website
    # will accept it as a PTB:
    ARTIFACT_NAME="Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}${BUILD_COUNTER_SUFFIX}-windows-64-installer.exe"
  else
    NAME_SUFFIX='_64_'
    INSTALLER_ICON_WINFILE=$(cygpath -aw "${GITHUB_WORKSPACE}/src/icons/mudlet.ico")
    ID='Mudlet_64_'
    TITLE='Mudlet x64'
    LOADING_GIF=$(cygpath -aw "${GITHUB_WORKSPACE}/installers/windows/splash-installing-2x.png")
    # Typically       '4.19.1'
    INSTALLER_VERSION="${VERSION}"
    # Typically of form: 'Mudlet-4.19.1-windows-64-installer.exe'
    INSTALLER_EXE="Mudlet-${VERSION}-windows-64-installer.exe"
    DBLSQD_VERSION_STRING="${VERSION}"
  fi
  ./Clowd.Squirrel/tools/Squirrel.exe pack \
    --allowUnaware \
    --noDelta \
    --packId="${ID}" \
    --packVersion="${INSTALLER_VERSION}" \
    --packAuthors='Mudlet Makers' \
    --packTitle="${TITLE}" \
    --packDir="$(cygpath -aw "${PACKAGE_PATH}")" \
    --splashImage="${LOADING_GIF}" \
    --icon="${INSTALLER_ICON_WINFILE}" \
    --releaseDir="${RELEASE_WINDIR}"

  # The above should produce, for both Release and PTBs SEVERAL files including
  # a 'Mudlet${NAME_SUFFIX}Setup.exe' in the ${RELEASE_DIR}:
  # Check if the expected 'setup' executable exists
  EXPECTED_SETUP_EXE="${RELEASE_DIR}/Mudlet${NAME_SUFFIX}Setup.exe"
  if [[ ! -f ${EXPECTED_SETUP_EXE} ]]; then
    echo "=== ERROR: Clowd.Squirrel failed to generate the installer ${RELEASE_DIR}/Mudlet${NAME_SUFFIX}Setup.exe! ==="
    echo 'Build aborted. Squirrel log is:'

    # Check if the Squirrel.log exists and display its content
    SQUIRREL_LOG_PATHFILE=$(cygpath -au "${LOCALAPPDATA}/SquirrelClowdTemp/Squirrel.log")
    if [[ -f ${SQUIRREL_LOG_PATHFILE} ]]; then
      echo "=== SquirrelSetup.log ==="
      cat "${SQUIRREL_LOG_PATHFILE}"
    else
      echo "  \"${SQUIRREL_LOG_PATHFILE}\" - not found"
    fi
    echo "=== End of SquirrelSetup.log ==="

    exit 9
  fi

  echo "=== Renaming installer ==="
  INSTALLER_EXE_WINPATHFILE="$(cygpath -aw "${RELEASE_DIR}/${INSTALLER_EXE}")"
  INSTALLER_EXE_PATHFILE="$(cygpath -au "${RELEASE_DIR}/${INSTALLER_EXE}")"
  echo "Renaming \"Mudlet${NAME_SUFFIX}Setup.exe\" to \"${INSTALLER_EXE}\""
  mv "${RELEASE_DIR}/Mudlet${NAME_SUFFIX}Setup.exe" "${INSTALLER_EXE_PATHFILE}"

  if [[ "${GITHUB_SCHEDULED_BUILD}" == "true" ]]; then
    echo "=== Preparing artifact for PTB for upload to make.mudlet.org ==="
    # Append these variables to the GITHUB_ENV to make them available in
    # subsequent steps, the fourth one being 1 means "unzip the archive when
    # it is uploaded to the Mudlet website":
    {
      echo "ARTIFACT_NAME=${ARTIFACT_NAME}"
      echo "ARTIFACT_WINPATHORFILE=${INSTALLER_EXE_WINPATHFILE}"
      echo "ARTIFACT_COMPRESSION=0"
      echo "ARTIFACT_UNZIP=1"
    } >> "${GITHUB_ENV}"

    # This identifies the "channel" that the release applies to, currently
    # we have three defined: this one; "release" and (unused) "testing":
    DBLSQD_CHANNEL="public-test-build"
    CHANGELOG_MODE="ptb"
  else
    # This is a Release
    echo "=== Uploading installer to https://www.mudlet.org/wp-content/files/?C=M;O=D ==="
    echo "${DEPLOY_SSH_KEY}" > temp_key_file

    echo "Fixing permissions of private key file"
    powershell.exe -Command "icacls.exe temp_key_file /inheritance:r"

    powershell.exe <<EOF
\$installerExePath = "${INSTALLER_EXE_WINPATHFILE}"
\$DEPLOY_PATH = "${DEPLOY_PATH}"
scp.exe -i temp_key_file -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null \$installerExePath mudmachine@mudlet.org:\${DEPLOY_PATH}
EOF

    shred -u temp_key_file

    DEPLOY_URL="https://www.mudlet.org/wp-content/files/Mudlet-${VERSION}-windows-64-installer.exe"

    if ! curl --output /dev/null --silent --head --fail "${DEPLOY_URL}"; then
      echo "Error: release not found as expected at ${DEPLOY_URL}"
      exit 1
    fi

    SHA256SUM=$(shasum -a 256 "${INSTALLER_EXE_PATHFILE}" | awk '{print $1}')

    current_timestamp=$(date "+%-d %-m %Y %-H %-M %-S")
    read -r DAY MONTH YEAR HOUR MINUTE SECOND <<< "${current_timestamp}"

    echo ""
    echo "=== Updating WP-Download-Manager ==="
    echo "sha256 of installer: ${SHA256SUM}"

    curl --retry 5 -X POST 'https://www.mudlet.org/download-add.php' \
    -H "x-wp-download-token: ${X_WP_DOWNLOAD_TOKEN}" \
    -F "file_type=2" \
    -F "file_remote=${DEPLOY_URL}" \
    -F "file_name=Mudlet ${VERSION} (windows-64)" \
    -F "file_des=sha256: ${SHA256SUM}" \
    -F "file_cat=2" \
    -F "file_permission=-1" \
    -F "file_timestamp_day=${DAY}" \
    -F "file_timestamp_month=${MONTH}" \
    -F "file_timestamp_year=${YEAR}" \
    -F "file_timestamp_hour=${HOUR}" \
    -F "file_timestamp_minute=${MINUTE}" \
    -F "file_timestamp_second=${SECOND}" \
    -F "output=json" \
    -F "do=Add File"

    echo "=== Uploading portable ZIP to mudlet.org ==="
    # Check if portable ZIP exists
    if [ "${MAKE_PORTABLE}" == 'true' ] && [ -f "${PORTABLE_ARTIFACT_PATHORFILE}" ]; then
      # Create SSH key file for portable upload
      echo "${DEPLOY_SSH_KEY}" > temp_key_file_portable
      powershell.exe -Command "icacls.exe temp_key_file_portable /inheritance:r"

      # Upload portable ZIP via SCP with proper naming
      powershell.exe <<EOF
\$PORTABLE_ARTIFACT_WINPATHORFILE = "${PORTABLE_ARTIFACT_WINPATHORFILE}"
\$DEPLOY_PATH = "${DEPLOY_PATH}"
\$PORTABLE_ARTIFACT_NAME = "${PORTABLE_ARTIFACT_NAME}"
scp.exe -i temp_key_file_portable -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null \$PORTABLE_ARTIFACT_WINPATHORFILE mudmachine@mudlet.org:\$DEPLOY_PATH/\$PORTABLE_ARTIFACT_NAME
EOF

      shred -u temp_key_file_portable

      # Define portable self-extracting archive URL - should match the naming
      # convention:
      PORTABLE_DEPLOY_URL="https://www.mudlet.org/wp-content/files/${PORTABLE_ARTIFACT_NAME}"

      # Verify portable ZIP was uploaded
      if ! curl --output /dev/null --silent --head --fail "${PORTABLE_DEPLOY_URL}"; then
        echo "Error: portable ZIP not found as expected at ${PORTABLE_DEPLOY_URL}"
        exit 1
      fi

      # Calculate SHA256 for portable ZIP
      PORTABLE_SHA256SUM=$(shasum -a 256 "${PORTABLE_ARTIFACT_PATHORFILE}" | awk '{print $1}')

      echo "=== Registering portable ZIP with WP-Download-Manager ==="
      echo "SHA256 of portable self-extracting archive: ${PORTABLE_SHA256SUM}"

      # Register portable self-extracting archive with download manager
      curl --retry 5 -X POST 'https://www.mudlet.org/download-add.php' \
      -H "x-wp-download-token: ${X_WP_DOWNLOAD_TOKEN}" \
      -F "file_type=2" \
      -F "file_remote=${PORTABLE_DEPLOY_URL}" \
      -F "file_name=Mudlet ${VERSION} Portable (windows-64)" \
      -F "file_des=sha256: ${PORTABLE_SHA256SUM}" \
      -F "file_cat=2" \
      -F "file_permission=-1" \
      -F "file_timestamp_day=${DAY}" \
      -F "file_timestamp_month=${MONTH}" \
      -F "file_timestamp_year=${YEAR}" \
      -F "file_timestamp_hour=${HOUR}" \
      -F "file_timestamp_minute=${MINUTE}" \
      -F "file_timestamp_second=${SECOND}" \
      -F "output=json" \
      -F "do=Add File"

      echo "Portable self-extracting archive uploaded and registered successfully"
    else
      echo "Warning: Portable self-extracting archive not found at ${PORTABLE_ARTIFACT_PATHORFILE}, skipping portable upload"
    fi

    DBLSQD_CHANNEL="release"
    CHANGELOG_MODE="release"
  fi

  echo "=== Installing NodeJS ==="
  # Check: according to https://github.com/actions/runner-images/blob/main/images/windows/Windows2022-Readme.md
  # we already have node 22.17.1 available to us:
  choco install --no-progress nodejs --version="22.1.0" -y -r -n
  PATH="/c/Program Files/nodejs/:/c/npm/prefix/:${PATH}"
  export PATH

  echo "=== Installing dblsqd-cli ==="
  npm install -g dblsqd-cli
  echo "=== Logging-in to dblsqd ==="
  dblsqd login -e "https://api.dblsqd.com/v1/jsonrpc" -u "${DBLSQD_USER}" -p "${DBLSQD_PASS}"

  echo "=== Downloading release feed ==="
  DOWNLOADED_FEED=$(mktemp)
  curl "https://feeds.dblsqd.com/MKMMR7HNSP65PquQQbiDIw/${DBLSQD_CHANNEL}/win/x86_64" -o "${DOWNLOADED_FEED}"

  echo "=== Generating a changelog ==="
  cd "${GITHUB_WORKSPACE}/CI" || exit 1

  GENERATE_CHANGELOG_FILEPATH="$(cygpath -a "${GITHUB_WORKSPACE}/CI/generate-changelog.lua")"
  CHANGELOG="$(lua5.1 "${GENERATE_CHANGELOG_FILEPATH}" --mode "${CHANGELOG_MODE}" --releasefile "${DOWNLOADED_FEED}")"
  # cd - seems to swap between the current and previous working directory!
  cd - || exit 1
  echo "=== Changelog ==="
  echo "${CHANGELOG}"
  echo "=== End of Changelog ==="

  echo "=== Creating release in Dblsqd ==="
  echo "DBLSQD_VERSION_STRING=\"${DBLSQD_VERSION_STRING}\""
  export DBLSQD_VERSION_STRING

  # This could once have failed as a build from another architecture (32 vs 64
  # bits) may have already registered a release with dblsqd, if so, that was OK
  # but is no longer an issue as we don't do 32 bits anymore.
  # Don't reproduce the changelog contents in the following echo - we've already
  # shown them:
  echo "dblsqd release -a mudlet -c ${DBLSQD_CHANNEL} -m \${CHANGELOG} \"${DBLSQD_VERSION_STRING}\""
  dblsqd release -a mudlet -c "${DBLSQD_CHANNEL}" -m "${CHANGELOG}" "${DBLSQD_VERSION_STRING}" || true

  # PTB's are handled by the register script, release builds are just pushed here
  if [[ "${DBLSQD_CHANNEL}" == "release" ]]; then
    echo "=== Registering release with Dblsqd ==="
    echo "dblsqd push -a mudlet -c \"${DBLSQD_CHANNEL}\" -r \"${DBLSQD_VERSION_STRING}\" -s mudlet --type 'standalone' --attach win:x86_64 \"${DEPLOY_URL}\""
    dblsqd push -a mudlet -c "${DBLSQD_CHANNEL}" -r "${DBLSQD_VERSION_STRING}" -s mudlet --type 'standalone' --attach win:x86_64 "${DEPLOY_URL}"
  fi

fi

# Make VERSION_STRING and BUILD_COMMIT available to the
# GHA "build-mudlet-win.yml" workflow so they can be passed to the
# "Register PTB Release" step:
{
  echo "VERSION_STRING=${DBLSQD_VERSION_STRING}"
  echo "BUILD_COMMIT=${BUILD_COMMIT}"
} >> "${GITHUB_ENV}"

echo ""
echo "******************************************************"
echo ""
if [[ "${BUILD_ACTION}" == "Release" ]]; then
  echo "Finished deploying Mudlet ${VERSION}"
else
  # Not a release build so include the Git SHA1 in the message
  echo "Finished deploying Mudlet ${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}${BUILD_COUNTER_SUFFIX}"
fi

if [[ -n "${DEPLOY_URL}" ]]; then
  echo "Deployed the installer to ${DEPLOY_URL}"
fi
if [[ -n "${PORTABLE_DEPLOY_URL}" ]]; then
  echo "Deployed the portable self-executable archive to ${PORTABLE_DEPLOY_URL}"
fi

echo ""
echo "******************************************************"
