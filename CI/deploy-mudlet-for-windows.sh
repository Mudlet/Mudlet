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
  export BUILD_BITNESS="64"
  export BUILDCOMPONENT="x86_64"
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

PublicTestBuild=false
# Check if GITHUB_REPO_TAG is "false"
if [[ "${GITHUB_REPO_TAG}" == "false" ]]; then
  echo "=== GITHUB_REPO_TAG is FALSE ==="

  # Check if this is a scheduled build
  if [[ "${GITHUB_SCHEDULED_BUILD}" == "true" ]]; then
    echo "=== GITHUB_SCHEDULED_BUILD is TRUE, this is a PTB ==="
    MUDLET_VERSION_BUILD="-ptb"
    PublicTestBuild=true
  else
    MUDLET_VERSION_BUILD="-testing"
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
      DATE=$(date +%F)
      MUDLET_VERSION_BUILD="${MUDLET_VERSION_BUILD}-${DATE}"
    fi
  fi
fi

# Convert to lowercase, not all systems deal with uppercase ASCII characters
export MUDLET_VERSION_BUILD="${MUDLET_VERSION_BUILD,,}"
export BUILD_COMMIT="${BUILD_COMMIT,,}"

# Extract version from the mudlet.pro file
VersionLine=$(grep "VERSION =" "${GITHUB_WORKSPACE}/src/mudlet.pro")
VersionRegex='= {1}(.+)$'

# Use Bash regex matching to extract version
if [[ ${VersionLine} =~ ${VersionRegex} ]]; then
  VERSION="${BASH_REMATCH[1]}"
fi

# Check if MUDLET_VERSION_BUILD is empty and print accordingly
if [[ -z "${MUDLET_VERSION_BUILD}" ]]; then
  # Possible release build
  echo "BUILDING MUDLET ${VERSION}"
else
  # Include Git SHA1 in the build information
  echo "BUILDING MUDLET ${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}"
fi

# Check if we're building from the Mudlet/Mudlet repository and not a fork
if [[ "${GITHUB_REPO_NAME}" != "Mudlet/Mudlet" ]]; then
  exit 2
fi

GITHUB_WORKSPACE_UNIX_PATH=$(echo "${GITHUB_WORKSPACE}" | sed 's|\\|/|g' | sed 's|D:|/d|g' | sed 's|C:|/c|g')
PACKAGE_DIR="${GITHUB_WORKSPACE_UNIX_PATH}/package-${MSYSTEM}-release"

cd "${PACKAGE_DIR}" || exit 1

# Remove specific file types from the directory
rm ./*.cpp ./*.o

# Helper function to move a packaged mudlet to the upload directory and set up an artifact upload
# We require the files to be uploaded to exist in ${PACKAGE_DIR}
moveToUploadDir() {
  local uploadFilename=$1
  local unzip=$2
  echo "=== Setting up upload directory ==="
  local uploadDir="${GITHUB_WORKSPACE}/upload"
  local uploadDirUnix
  uploadDirUnix=$(echo "${uploadDir}" | sed 's|\\|/|g' | sed 's|D:|/d|g' | sed 's|C:|/c|g')

  # Check if the upload directory exists, if not, create it
  if [[ ! -d "${uploadDirUnix}" ]]; then
    mkdir -p "${uploadDirUnix}"
  fi

  echo "=== Copying files to upload directory ==="
  rsync -avR "${PACKAGE_DIR}"/./* "${uploadDirUnix}"

  # Append these variables to the GITHUB_ENV to make them available in subsequent steps
  {
    echo "FOLDER_TO_UPLOAD=${uploadDir}/"
    echo "UPLOAD_FILENAME=${uploadFilename}"
    echo "PARAM_UNZIP=${unzip}"
  } >> "${GITHUB_ENV}"
}

# Check if GITHUB_REPO_TAG and PublicTestBuild are "false" for a snapshot build
if [[ "${GITHUB_REPO_TAG}" == "false" ]] && [[ "${PublicTestBuild}" == false ]]; then
  echo "=== Creating a snapshot build ==="
  mv "${PACKAGE_DIR}/mudlet.exe" "Mudlet.exe"

  # Define the upload filename
  uploadFilename="Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-windows-${BUILD_BITNESS}"

  # Move packaged files to the upload directory
  moveToUploadDir "${uploadFilename}" 0
else

  # Check if it's a Public Test Build
  if [[ "${PublicTestBuild}" == "true" ]]; then

    # Get the commit date of the last commit
    COMMIT_DATE=$(git show -s --format="%cs")
    # Get yesterday's date in the same format
    YESTERDAY_DATE=$(date --date="yesterday" +%Y-%m-%d)

    if [[ "${COMMIT_DATE}" < "${YESTERDAY_DATE}" ]]; then
      echo "=== No new commits, aborting public test build generation ==="
      exit 0
    fi

    echo "=== Creating a public test build ==="
    # Squirrel uses Start menu name from the binary, renaming it
    mv "${PACKAGE_DIR}/mudlet.exe" "${PACKAGE_DIR}/Mudlet PTB.exe"
    echo "moved mudlet.exe to ${PACKAGE_DIR}/Mudlet PTB.exe"
    # ensure sha part always starts with a character due to a known issue
    VersionAndSha="${VERSION}-ptb-${BUILD_COMMIT}"

  else
    echo "=== Creating a release build ==="
    mv "${PACKAGE_DIR}/mudlet.exe" "${PACKAGE_DIR}/Mudlet.exe"
    VersionAndSha="${VERSION}"
  fi

  echo "VersionAndSha: ${VersionAndSha}"
  echo "=== Cloning installer project ==="
  git clone https://github.com/Mudlet/installers.git "${GITHUB_WORKSPACE}/installers"
  cd "${GITHUB_WORKSPACE}/installers/windows" || exit 1

  echo "=== Setting up Java 21 for signing ==="
  JAVA_HOME="$(cygpath -u "${JAVA_HOME_21_X64}")"
  export JAVA_HOME
  export PATH="${JAVA_HOME}/bin:${PATH}"

  if [ -z "${AZURE_ACCESS_TOKEN}" ]; then
      echo "=== Code signing skipped - no Azure token provided ==="
  else
      echo "=== Signing Mudlet and dll files ==="
      if [[ "${PublicTestBuild}" == "true" ]]; then
          java.exe -jar "${GITHUB_WORKSPACE}/installers/windows/jsign-7.0-SNAPSHOT.jar" --storetype TRUSTEDSIGNING \
              --keystore eus.codesigning.azure.net \
              --storepass "${AZURE_ACCESS_TOKEN}" \
              --alias Mudlet/Mudlet \
              "${PACKAGE_DIR}/Mudlet PTB.exe" "${PACKAGE_DIR}/**/*.dll"
      else
          java.exe -jar "${GITHUB_WORKSPACE}/installers/windows/jsign-7.0-SNAPSHOT.jar" --storetype TRUSTEDSIGNING \
              --keystore eus.codesigning.azure.net \
              --storepass "${AZURE_ACCESS_TOKEN}" \
              --alias Mudlet/Mudlet \
              "${PACKAGE_DIR}/Mudlet.exe" "${PACKAGE_DIR}/**/*.dll"
      fi
  fi

  echo "=== Installing Squirrel for Windows ==="
  nuget install squirrel.windows -ExcludeVersion

  echo "=== Setting up directories ==="
  SQUIRRELWIN="${GITHUB_WORKSPACE}/squirrel-packaging-prep"
  SQUIRRELWINBIN="${SQUIRRELWIN}/lib/net45/"

  if [[ ! -d "${SQUIRRELWINBIN}" ]]; then
    mkdir -p "${SQUIRRELWINBIN}"
  fi

  echo "=== Moving things to where Squirrel expects them ==="
  mv "${PACKAGE_DIR}/"* "${SQUIRRELWINBIN}"

  # Set the path to the nuspec file
  NuSpec="${GITHUB_WORKSPACE}/installers/windows/mudlet.nuspec"
  echo "=== Creating Nuget package ==="

  # Rename the id and title for Squirrel
  if [[ "${PublicTestBuild}" == "true" ]]; then
    # Allow public test builds to be installed side by side with the release builds by renaming the app
    # No dots in the <id>: Guidelines by Squirrel
    if [ "${MSYSTEM}" = "MINGW64" ]; then
      sed -i "s/<id>Mudlet<\/id>/<id>Mudlet_${BUILD_BITNESS}_-PublicTestBuild<\/id>/" "${NuSpec}"
    else
      sed -i 's/<id>Mudlet<\/id>/<id>Mudlet-PublicTestBuild<\/id>/' "${NuSpec}"
    fi
    sed -i "s/<title>Mudlet<\/title>/<title>Mudlet x${BUILD_BITNESS} (Public Test Build)<\/title>/" "${NuSpec}"
  else
    if [ "${MSYSTEM}" = "MINGW64" ]; then
      sed -i "s/<id>Mudlet<\/id>/<id>Mudlet_${BUILD_BITNESS}_<\/id>/" "${NuSpec}"
    fi
    sed -i "s/<title>Mudlet<\/title>/<title>Mudlet x${BUILD_BITNESS}<\/title>/" "${NuSpec}"
  fi

  # Create NuGet package
  nuget pack "${NuSpec}" -Version "${VersionAndSha}" -BasePath "${SQUIRRELWIN}" -OutputDirectory "${SQUIRRELWIN}"

  echo "=== Preparing to create installer ==="
  if [[ "${PublicTestBuild}" == "true" ]]; then
    TestBuildString="-PublicTestBuild"
    InstallerIconFile="${GITHUB_WORKSPACE}/src/icons/mudlet_ptb.ico"
  else
    TestBuildString=""
    InstallerIconFile="${GITHUB_WORKSPACE}/src/icons/mudlet.ico"
  fi

  # Ensure 64 bit build is properly tagged
  if [ "${MSYSTEM}" = "MINGW64" ]; then
    TestBuildString="_64_${TestBuildString}"
  fi

  nupkg_path="${GITHUB_WORKSPACE}/squirrel-packaging-prep/Mudlet${TestBuildString}.${VersionAndSha}.nupkg"
  if [[ ! -f "${nupkg_path}" ]]; then
    echo "=== ERROR: nupkg doesn't exist as expected! Build aborted."
    exit 4
  fi

  # Execute Squirrel to create the installer
  echo "=== Creating installers from Nuget package ==="
  ./squirrel.windows/tools/Squirrel --releasify "${nupkg_path}" \
    --releaseDir "${GITHUB_WORKSPACE}/squirreloutput" \
    --loadingGif "${GITHUB_WORKSPACE}/installers/windows/splash-installing-2x.png" \
    --no-msi --setupIcon "${InstallerIconFile}"
    
  echo "=== Removing old directory content of release folder ==="
  rm -rf "${PACKAGE_DIR:?}/*"

  echo "=== Copying installer over ==="
  if [[ "${PublicTestBuild}" == "true" ]]; then
    installerExePath="${PACKAGE_DIR}/Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-windows-${BUILD_BITNESS}.exe"
  else # release
    installerExePath="${PACKAGE_DIR}/Mudlet-${VERSION}-windows-${BUILD_BITNESS}-installer.exe"
  fi
  echo  "installerExePath: ${installerExePath}"
  mv "${GITHUB_WORKSPACE}/squirreloutput/Setup.exe" "${installerExePath}"

  # Sign the final installer
  echo "=== Signing installer ==="
  java.exe -jar "${GITHUB_WORKSPACE}/installers/windows/jsign-7.0-SNAPSHOT.jar" --storetype TRUSTEDSIGNING \
      --keystore eus.codesigning.azure.net \
      --storepass "${AZURE_ACCESS_TOKEN}" \
      --alias Mudlet/Mudlet \
      "${installerExePath}"

  # Check if the setup executable exists
  if [[ ! -f "${installerExePath}" ]]; then
    echo "=== ERROR: Squirrel failed to generate the installer! Build aborted. Squirrel log is:"

    # Check if the SquirrelSetup.log exists and display its content
    if [[ -f "./squirrel.windows/tools/SquirrelSetup.log" ]]; then
      echo "SquirrelSetup.log: "
      cat "./squirrel.windows/tools/SquirrelSetup.log"
    fi

    # Check if the Squirrel-Releasify.log exists and display its content
    if [[ -f "./squirrel.windows/tools/Squirrel-Releasify.log" ]]; then
      echo "Squirrel-Releasify.log: "
      cat "./squirrel.windows/tools/Squirrel-Releasify.log"
    fi

    exit 5
  fi

  if [[ "${PublicTestBuild}" == "true" ]]; then
    echo "=== Uploading public test build to make.mudlet.org ==="

    uploadFilename="Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-windows-${BUILD_BITNESS}-installer.exe"
    echo "uploadFilename: ${uploadFilename}"

    # Installer named ${uploadFilename} should exist in ${PACKAGE_DIR} now, we're ok to proceed
    moveToUploadDir "${uploadFilename}" 1
    RELEASE_TAG="public-test-build"
    CHANGELOG_MODE="ptb"
  else

    echo "=== Uploading installer to https://www.mudlet.org/wp-content/files/?C=M;O=D ==="
    echo "${DEPLOY_SSH_KEY}" > temp_key_file

    # chown doesn't work in msys2 and scp requires the not be globally readable
    # use a powershell workaround to set the permissions correctly
    echo "Fixing permissions of private key file"
    powershell.exe -Command "icacls.exe temp_key_file /inheritance:r"

    powershell.exe <<EOF
\$installerExePath = "${installerExePath}"
\$DEPLOY_PATH = "${DEPLOY_PATH}"
scp.exe -i temp_key_file -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null \$installerExePath mudmachine@mudlet.org:\${DEPLOY_PATH}
EOF

    shred -u temp_key_file

    DEPLOY_URL="https://www.mudlet.org/wp-content/files/Mudlet-${VERSION}-windows-${BUILD_BITNESS}-installer.exe"

    if ! curl --output /dev/null --silent --head --fail "${DEPLOY_URL}"; then
      echo "Error: release not found as expected at ${DEPLOY_URL}"
      exit 1
    fi

    SHA256SUM=$(shasum -a 256 "${installerExePath}" | awk '{print $1}')

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
    -F "file_name=Mudlet ${VERSION} (windows-${BUILD_BITNESS})" \
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
    
    RELEASE_TAG="release"
    CHANGELOG_MODE="release"
  fi

  echo "=== Installing NodeJS ==="
  choco install --no-progress nodejs --version="22.1.0" -y -r -n
  PATH="/c/Program Files/nodejs/:/c/npm/prefix/:${PATH}"
  export PATH

  echo "=== Installing dblsqd-cli ==="
  npm install -g dblsqd-cli
  dblsqd login -e "https://api.dblsqd.com/v1/jsonrpc" -u "${DBLSQD_USER}" -p "${DBLSQD_PASS}"

  echo "=== Downloading release feed ==="
  DownloadedFeed=$(mktemp)
  curl "https://feeds.dblsqd.com/MKMMR7HNSP65PquQQbiDIw/${RELEASE_TAG}/win/${ARCH}" -o "${DownloadedFeed}"

  echo "=== Generating a changelog ==="
  cd "${GITHUB_WORKSPACE}/CI" || exit 1
  
  Changelog=$(lua5.1 "${GITHUB_WORKSPACE}/CI/generate-changelog.lua" --mode "${CHANGELOG_MODE}" --releasefile "${DownloadedFeed}")
  cd - || exit 1
  echo "${Changelog}"

  echo "=== Creating release in Dblsqd ==="
  if [[ "${PublicTestBuild}" == "true" ]]; then
    VersionString="${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT,,}"
  else # release
    VersionString="${VERSION}"
  fi
  
  echo "VersionString: ${VERSION}String"
  export VersionString

  # This may fail as a build from another architecture may have already registered a release with dblsqd,
  # if so, that is OK...
  echo "dblsqd release -a mudlet -c ${RELEASE_TAG} -m \"${Changelog}\" \"${VersionString}\""
  dblsqd release -a mudlet -c "${RELEASE_TAG}" -m "${Changelog}" "${VersionString}" || true

  # PTB's are handled by the register script, release builds are just pushed here
  if [[ "${RELEASE_TAG}" == "release" ]]; then
    echo "=== Registering release with Dblsqd ==="
    echo "dblsqd push -a mudlet -c release -r \"${VersionString}\" -s mudlet --type 'standalone' --attach win:${ARCH} \"${DEPLOY_URL}\""
    dblsqd push -a mudlet -c release -r "${VersionString}" -s mudlet --type 'standalone' --attach win:"${ARCH}" "${DEPLOY_URL}"
  fi

fi

# Make PublicTestBuild available GHA to check if we need to run the register step
{
  echo "PUBLIC_TEST_BUILD=${PublicTestBuild}"
  echo "ARCH=${ARCH}"
  echo "VERSION_STRING=${VersionString}"
  echo "BUILD_COMMIT=${BUILD_COMMIT}"
} >> "${GITHUB_ENV}"

echo ""
echo "******************************************************"
echo ""
if [[ -z "${MUDLET_VERSION_BUILD}" ]]; then
  # A release build
  echo "Finished building Mudlet ${VERSION}"
else
  # Not a release build so include the Git SHA1 in the message
  echo "Finished building Mudlet ${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}"
fi

if [[ -n "${DEPLOY_URL}" ]]; then
  echo "Deployed the output to ${DEPLOY_URL}"
fi

echo ""
echo "******************************************************"
