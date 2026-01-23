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

# Version: 2.1.0    Remove MINGW32 since upstream no longer supports it
#          2.0.0    Rework to build on an MSYS2 MINGW64 Github workflow

# Exit codes:
# 0 - Everything is fine. 8-)
# 1 - Failure to change to a directory
# 2 - Unsupported fork
# 3 - Not used
# 4 - nuget error
# 5 - squirrel error

if [ "${MSYSTEM}" = "MSYS" ]; then
  echo "Please run this script from a MINGW64 type bash terminal as the MSYS one"
  echo "does not supported what is needed."
  exit 2
elif [ "${MSYSTEM}" = "MINGW64" ]; then
  export BUILDCOMPONENT="x86_64"
  # We only support "x86_64" architecture now but we used to do "x86" (32-bit)
  # as well and exported this value as ARCH for use here and in other scripts
else
  echo "This script is not set up to handle systems of type ${MSYSTEM}, only"
  echo "MINGW64 is currently supported. Please rerun this in a bash terminal of"
  echo "that type."
  exit 2
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

# Extract version from the mudlet.pro file
VERSION_LINE=$(grep "VERSION =" "${GITHUB_WORKSPACE}/src/mudlet.pro")
VERSION_REGEX='= {1}(.+)$'

# Use Bash regex matching to extract version - don't double-quote these as that
# can mess things up!
if [[ ${VERSION_LINE} =~ ${VERSION_REGEX} ]]; then
  VERSION="${BASH_REMATCH[1]}"
fi

# Check if MUDLET_VERSION_BUILD is empty and print accordingly
if [[ -z "${MUDLET_VERSION_BUILD}" ]]; then
  # Probably a release build - so typical output could be:
  #    "BUILDING MUDLET 4.19.1
  echo "BUILDING MUDLET ${VERSION}"
else
  # Include Git SHA1 in the build information
  # Probably a PTB - so typical output could be:
  #    "BUILDING MUDLET 4.19.1-ptb-2025-01-01-012345678
  echo "BUILDING MUDLET ${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}"
fi

# Check if we're building from the Mudlet/Mudlet repository and not a fork
if [[ "${GITHUB_REPO_NAME}" != "Mudlet/Mudlet" ]]; then
  exit 2
fi

# This will change to end in "-debug" if we ever do that type of build:
PACKAGE_PATH="$(cygpath -au "${GITHUB_WORKSPACE}/package-${MSYSTEM}-release")"
PACKAGE_WINPATH="$(cygpath -aw "${PACKAGE_PATH}")"
cd "${PACKAGE_PATH}" || exit 1

# Check if GITHUB_REPO_TAG and GITHUB_SCHEDULED_BUILD are not "true" for a snapshot build
if [[ "${GITHUB_REPO_TAG}" != "true" ]] && [[ "${GITHUB_SCHEDULED_BUILD}" != "true" ]]; then
  echo "=== Creating a snapshot build ==="
  PACKAGE_EXE="Mudlet.exe"
  mv "${PACKAGE_PATH}/mudlet.exe" "${PACKAGE_PATH}/${PACKAGE_EXE}"

  # Define the upload filename - MUDLET_VERSION_BUILD will at least be something
  # like "-testing" or "-testing-pr####" but NOT "-ptb-*"
  # THIS IS THE NAME GIVEN TO THE GHA "artifact" which is automagically made
  # as a zip archive file.
  ARTIFACT_NAME="Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-windows-64"
  ARTIFACT_WINPATHORFILE="$(cygpath -aw "${PACKAGE_PATH}")"
  # Append these variables to the GITHUB_ENV to make them available in
  # subsequent steps, the fourth one being 0 means "don't unzip the archive when
  # it is uploaded to the Mudlet website". In this place and further down when
  # appending to the GH Actions environment DO NOT add escaped double-quotes
  # around the string after the '=' such extra double quotes
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

    # Squirrel uses the name of the binary for the Start menu, so need to rename
    # it:
    PACKAGE_EXE="Mudlet PTB.exe"
  else
    echo "=== Creating a release build ==="
    PACKAGE_EXE="Mudlet.exe"
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

  echo "=== Setting up Java 21 for signing ==="
  # Java is installed by default, we just need to select which version to use:
  JAVA_HOME="$(cygpath -au "${JAVA_HOME_21_X64}")"
  export JAVA_HOME
  export PATH="${JAVA_HOME}/bin:${PATH}"
  JAVA_JAR_WINPATHFILE="$(cygpath -aw "${GITHUB_WORKSPACE}/installers/windows/jsign-7.0-SNAPSHOT.jar")"

  if [ -z "${AZURE_ACCESS_TOKEN}" ]; then
    echo "=== Code signing of Mudlet application and bundled libraries skipped - no Azure token provided ==="
  else
    echo "=== Signing Mudlet executable and bundled libraries ==="
    java.exe -jar "${JAVA_JAR_WINPATHFILE}" \
      --storetype TRUSTEDSIGNING \
      --keystore eus.codesigning.azure.net \
      --storepass "${AZURE_ACCESS_TOKEN}" \
      --alias Mudlet/Mudlet \
      "${PACKAGE_EXE_WINPATHFILE}" "${PACKAGE_WINPATH}\\**\\*.dll"
  fi

  echo "=== Preparing an intermediate artifact of the (signed) code ==="
  # What will it be called:
  if [[ -z "${MUDLET_VERSION_BUILD}" ]]; then
    INTERMEDIATE_ARTIFACT_NAME="Mudlet-${VERSION}-windows-64"
  else
    INTERMEDIATE_ARTIFACT_NAME="Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-windows-64"
  fi
  # This intermediate will NOT be uploaded but will remain on the GH server as
  # an artifact for a default (90?) days
  INTERMEDIATE_ARTIFACT_WINPATHORFILE="$(cygpath -aw "${PACKAGE_PATH}\\")"
  {
    echo "INTERMEDIATE_ARTIFACT_NAME=${INTERMEDIATE_ARTIFACT_NAME}"
    echo "INTERMEDIATE_ARTIFACT_WINPATHORFILE=${INTERMEDIATE_ARTIFACT_WINPATHORFILE}"
    echo "INTERMEDIATE_ARTIFACT_COMPRESSION=9"
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
    TITLE='Mudlet x64 (Public Test Build)'
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
    INSTALLER_VERSION="${VERSION}-ptb-${BUILD_COMMIT,,}"
    # The name we want to use for the installer;
    # Typically of form: 'Mudlet-4.19.1-ptb-2025-01-01-012345678-windows-64.exe'
    INSTALLER_EXE="Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-windows-64.exe"
    DBLSQD_VERSION_STRING="${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT,,}"
    # The name that has to be passed as the artifact so that the Mudlet website
    # will accept it as a PTB:
    ARTIFACT_NAME="Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-windows-64-installer.exe"
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

    exit 5
  fi

  echo "=== Renaming installer ==="
  INSTALLER_EXE_WINPATHFILE="$(cygpath -aw "${RELEASE_DIR}/${INSTALLER_EXE}")"
  INSTALLER_EXE_PATHFILE="$(cygpath -au "${RELEASE_DIR}/${INSTALLER_EXE}")"
  echo "Renaming \"Mudlet${NAME_SUFFIX}Setup.exe\" to \"${INSTALLER_EXE}\""
  mv "${RELEASE_DIR}/Mudlet${NAME_SUFFIX}Setup.exe" "${INSTALLER_EXE_PATHFILE}"

  # Sign the final installer
  if [ -z "${AZURE_ACCESS_TOKEN}" ]; then
    echo "=== Code signing of Mudlet installer skipped - no Azure token provided ==="
  else
    echo "=== Signing installer ==="
    java.exe -jar "${JAVA_JAR_WINPATHFILE}" \
      --storetype TRUSTEDSIGNING \
      --keystore eus.codesigning.azure.net \
      --storepass "${AZURE_ACCESS_TOKEN}" \
      --alias Mudlet/Mudlet \
      "${INSTALLER_EXE_WINPATHFILE}"
  fi

  if [[ "${GITHUB_SCHEDULED_BUILD}" == "true" ]]; then
    echo "=== Preparing artifact for PTB for upload to make.mudlet.org ==="
    # Copy the signed installer to a separate directory - as ${RELEASE_WINDIR}
    # will contain other files we do not want to upload:
    UPLOAD_PATH=$(cygpath -au "${GITHUB_WORKSPACE}/upload")
    mkdir -p "${UPLOAD_PATH}"
    cp -vp "${INSTALLER_EXE_PATHFILE}" "${UPLOAD_PATH}"
    # Append these variables to the GITHUB_ENV to make them available in
    # subsequent steps, the fourth one being 1 means "unzip the archive when
    # it is uploaded to the Mudlet website":
    ARTIFACT_WINPATHORFILE="$(cygpath -aw "${UPLOAD_PATH}")"
    {
      echo "ARTIFACT_NAME=${ARTIFACT_NAME}"
      echo "ARTIFACT_WINPATHORFILE=${ARTIFACT_WINPATHORFILE}"
      echo "ARTIFACT_COMPRESSION=0"
      echo "ARTIFACT_UNZIP=1"
    } >> "${GITHUB_ENV}"

    # This identifies the "channel" that the release applies to, currently
    # we have three defined: this one; "release" and (unused) "testing":
    DBLSQD_CHANNEL="public-test-build"
    CHANGELOG_MODE="ptb"
  else

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
    read -r day month year hour minute second <<< "${current_timestamp}"

    # blank echo to remove the stray 'PS D:\a\Mudlet\Mudlet\installers\windows> ' that shows up otherwise

    echo ""
    echo "=== Updating WP-Download-Manager ==="
    echo "sha256 of installer: ${SHA256SUM}"

    FILE_CATEGORY="2"

    current_timestamp=$(date "+%-d %-m %Y %-H %-M %-S")
    read -r day month year hour minute second <<< "${current_timestamp}"

    curl --retry 5 -X POST 'https://www.mudlet.org/download-add.php' \
    -H "x-wp-download-token: ${X_WP_DOWNLOAD_TOKEN}" \
    -F "file_type=2" \
    -F "file_remote=${DEPLOY_URL}" \
    -F "file_name=Mudlet ${VERSION} (windows-64)" \
    -F "file_des=sha256: ${SHA256SUM}" \
    -F "file_cat=${FILE_CATEGORY}" \
    -F "file_permission=-1" \
    -F "file_timestamp_day=${day}" \
    -F "file_timestamp_month=${month}" \
    -F "file_timestamp_year=${year}" \
    -F "file_timestamp_hour=${hour}" \
    -F "file_timestamp_minute=${minute}" \
    -F "file_timestamp_second=${second}" \
    -F "output=json" \
    -F "do=Add File"

    echo "=== Uploading portable ZIP to mudlet.org ==="
    # Define portable ZIP paths
    PORTABLE_ZIP_NAME="Mudlet-portable-${MSYSTEM,,}.zip"
    PORTABLE_ZIP_PATH="${GITHUB_WORKSPACE_UNIX_PATH}/${PORTABLE_ZIP_NAME}"

    # Check if portable ZIP exists
    if [[ -f "${PORTABLE_ZIP_PATH}" ]]; then
      echo "Found portable ZIP at: ${PORTABLE_ZIP_PATH}"

      # Create SSH key file for portable upload
      echo "${DEPLOY_SSH_KEY}" > temp_key_file_portable
      powershell.exe -Command "icacls.exe temp_key_file_portable /inheritance:r"

      # Upload portable ZIP via SCP with proper naming
      PORTABLE_REMOTE_NAME="Mudlet-${VERSION}-windows-${BUILD_BITNESS}-portable.zip"
      powershell.exe <<EOF
\$portableZipPath = "${PORTABLE_ZIP_PATH}"
\$DEPLOY_PATH = "${DEPLOY_PATH}"
\$remoteFileName = "${PORTABLE_REMOTE_NAME}"
scp.exe -i temp_key_file_portable -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null \$portableZipPath mudmachine@mudlet.org:\${DEPLOY_PATH}/\$remoteFileName
EOF

      shred -u temp_key_file_portable

      # Define portable ZIP URL - should match the naming convention
      PORTABLE_DEPLOY_URL="https://www.mudlet.org/wp-content/files/Mudlet-${VERSION}-windows-${BUILD_BITNESS}-portable.zip"

      # Verify portable ZIP was uploaded
      if ! curl --output /dev/null --silent --head --fail "${PORTABLE_DEPLOY_URL}"; then
        echo "Error: portable ZIP not found as expected at ${PORTABLE_DEPLOY_URL}"
        exit 1
      fi

      # Calculate SHA256 for portable ZIP
      PORTABLE_SHA256SUM=$(shasum -a 256 "${PORTABLE_ZIP_PATH}" | awk '{print $1}')

      echo "=== Registering portable ZIP with WP-Download-Manager ==="
      echo "sha256 of portable ZIP: ${PORTABLE_SHA256SUM}"

      # Register portable ZIP with download manager
      curl --retry 5 -X POST 'https://www.mudlet.org/download-add.php' \
      -H "x-wp-download-token: ${X_WP_DOWNLOAD_TOKEN}" \
      -F "file_type=2" \
      -F "file_remote=${PORTABLE_DEPLOY_URL}" \
      -F "file_name=Mudlet ${VERSION} Portable (windows-${BUILD_BITNESS})" \
      -F "file_des=sha256: ${PORTABLE_SHA256SUM}" \
      -F "file_cat=${FILE_CATEGORY}" \
      -F "file_permission=-1" \
      -F "file_timestamp_day=${day}" \
      -F "file_timestamp_month=${month}" \
      -F "file_timestamp_year=${year}" \
      -F "file_timestamp_hour=${hour}" \
      -F "file_timestamp_minute=${minute}" \
      -F "file_timestamp_second=${second}" \
      -F "output=json" \
      -F "do=Add File"

      echo "Portable ZIP uploaded and registered successfully"
    else
      echo "Warning: Portable ZIP not found at ${PORTABLE_ZIP_PATH}, skipping portable upload"
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

  # This may fail as a build from another architecture may have already registered a release with dblsqd,
  # if so, that is OK. Don't reproduce the changelog contents in the following
  # echo - we've already shown them:
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
# "Register Release" step:
{
  echo "VERSION_STRING=${DBLSQD_VERSION_STRING}"
  echo "BUILD_COMMIT=${BUILD_COMMIT}"
} >> "${GITHUB_ENV}"

echo ""
echo "******************************************************"
echo ""
if [[ -z "${MUDLET_VERSION_BUILD}" ]]; then
  # A release build
  echo "Finished deploying Mudlet ${VERSION}"
else
  # Not a release build so include the Git SHA1 in the message
  echo "Finished deploying Mudlet ${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}"
fi

if [[ -n "${DEPLOY_URL}" ]]; then
  echo "Deployed the output to ${DEPLOY_URL}"
fi

echo ""
echo "******************************************************"
