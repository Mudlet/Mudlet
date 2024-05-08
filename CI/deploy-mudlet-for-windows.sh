#!/bin/bash
###########################################################################
#   Copyright (C) 2024-2024  by John McKisson - john.mckisson@gmail.com   #
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

# Version: 2.0.0    Rework to build on an MSYS2 MINGW64 Github workflow

# Exit codes:
# 0 - Everything is fine. 8-)
# 1 - Failure to change to a directory
# 2 - Unsupported fork
# 3 - No new commits for PTB
# 4 - nuget error
# 5 - squirrel error

if [ "${MSYSTEM}" = "MSYS" ]; then
  echo "Please run this script from an MINGW32 or MINGW64 type bash terminal appropriate"
  echo "to the bitness you want to work on. You may do this once for each of them should"
  echo "you wish to do both."
  exit 2
elif [ "${MSYSTEM}" = "MINGW32" ]; then
  export BUILD_BITNESS="32"
  export BUILDCOMPONENT="i686"
  export DBLSQDTYPE="x86"
elif [ "${MSYSTEM}" = "MINGW64" ]; then
  export BUILD_BITNESS="64"
  export BUILDCOMPONENT="x86_64"
  export DBLSQDTYPE="x86_64"
else
  echo "This script is not set up to handle systems of type ${MSYSTEM}, only MINGW32 or"
  echo "MINGW64 are currently supported. Please rerun this in a bash terminal of one"
  echo "of those two types."
  exit 2
fi

cd "$GITHUB_WORKSPACE" || exit 1

PublicTestBuild=false
# Check if GITHUB_REPO_TAG is "false"
if [[ "$GITHUB_REPO_TAG" == "false" ]]; then
  echo "=== GITHUB_REPO_TAG is FALSE ==="

  # Check if this is a scheduled build
  if [[ "$GITHUB_SCHEDULED_BUILD" == "true" ]]; then
    echo "=== GITHUB_SCHEDULED_BUILD is TRUE, this is a PTB ==="
    MUDLET_VERSION_BUILD="-ptb"
    PublicTestBuild=true
  else
    MUDLET_VERSION_BUILD="-testing"
  fi

  # Check if this is a pull request
  if [[ -n "$GITHUB_PULL_REQUEST_NUMBER" ]]; then
    # Use the specific commit SHA from the pull request head, since GitHub Actions merges the PR
    BUILD_COMMIT=$(git rev-parse --short "$GITHUB_PULL_REQUEST_HEAD_SHA")
    MUDLET_VERSION_BUILD="$MUDLET_VERSION_BUILD-PR$GITHUB_PULL_REQUEST_NUMBER"
  else
    BUILD_COMMIT=$(git rev-parse --short HEAD)

    if [[ "$MUDLET_VERSION_BUILD" == "-ptb" ]]; then
      # Get current date in YYYY-MM-DD format
      DATE=$(date +%F)
      MUDLET_VERSION_BUILD="$MUDLET_VERSION_BUILD-$DATE"
    fi
  fi
fi

# Convert to lowercase, not all systems deal with uppercase ASCII characters
export MUDLET_VERSION_BUILD="${MUDLET_VERSION_BUILD,,}"
export BUILD_COMMIT="${BUILD_COMMIT,,}"

# Extract version from the mudlet.pro file
VersionLine=$(grep "VERSION =" "$GITHUB_WORKSPACE/src/mudlet.pro")
VersionRegex='= {1}(.+)$'

# Use Bash regex matching to extract version
if [[ $VersionLine =~ $VersionRegex ]]; then
  VERSION="${BASH_REMATCH[1]}"
fi

# Check if MUDLET_VERSION_BUILD is empty and print accordingly
if [[ -z "$MUDLET_VERSION_BUILD" ]]; then
  # Possible release build
  echo "BUILDING MUDLET $VERSION"
else
  # Include Git SHA1 in the build information
  echo "BUILDING MUDLET $VERSION$MUDLET_VERSION_BUILD-$BUILD_COMMIT"
fi

# Check if we're building from the Mudlet/Mudlet repository and not a fork
if [[ "$GITHUB_REPO_NAME" != "Mudlet/Mudlet" ]]; then
  exit 2
fi

GITHUB_WORKSPACE_UNIX_PATH=$(echo "${GITHUB_WORKSPACE}" | sed 's|\\|/|g' | sed 's|D:|/d|g')
PACKAGE_DIR="${GITHUB_WORKSPACE_UNIX_PATH}/package-${MSYSTEM}-release"

cd "$PACKAGE_DIR" || exit 1

moveToUploadDir() {
  local uploadFilename=$1
  echo "=== Setting up upload directory ==="
  local uploadDir="${GITHUB_WORKSPACE_UNIX_PATH}/upload"

  # Check if the upload directory exists, if not, create it
  if [[ ! -d "$uploadDir" ]]; then
    mkdir -p "$uploadDir"
  fi
  
  echo "=== Listing files in package directory ==="
  ls "${PACKAGE_DIR}"

  echo "=== Copying files to upload directory ==="
  #cp "${PACKAGE_DIR}/*" "$uploadDir/"
  #rsync -avR "${PACKAGE_DIR}"/./* "$uploadDir"
  mv "${PACKAGE_DIR}" "$uploadDir/"
  echo "=== Listing files in upload directory ==="
  ls "$uploadDir"

  # Append these variables to the GITHUB_ENV to make them available in subsequent steps
  echo "FOLDER_TO_UPLOAD=$uploadDir" >> "$GITHUB_ENV"
  echo "UPLOAD_FILENAME=$uploadFilename" >> "$GITHUB_ENV"
}


# Remove specific file types from the directory
rm ./*.cpp ./*.o


# Check if GITHUB_REPO_TAG is "false" and PublicTestBuild is not true
if [[ "$GITHUB_REPO_TAG" == "false" ]] && [[ "$PublicTestBuild" == false ]]; then
  echo "=== Creating a snapshot build ==="
  mv "$PACKAGE_DIR/mudlet.exe" "Mudlet.exe"
  
  echo "=== Setting up upload directory ==="
  uploadDir="${GITHUB_WORKSPACE}\\upload"
  uploadDirUnix=$(echo "${uploadDir}" | sed 's|\\|/|g' | sed 's|D:|/d|g')

  # Check if the upload directory exists, if not, create it
  if [[ ! -d "$uploadDirUnix" ]]; then
    mkdir -p "$uploadDirUnix"
  fi
  
  # Create a zip file using 7z
  #7z a "Mudlet-$VERSION$MUDLET_VERSION_BUILD-$BUILD_COMMIT-windows-$BUILD_BITNESS" "$PACKAGE_DIR/*"
  #echo "=== Listing files in package directory ==="
  #ls "${PACKAGE_DIR}"
  rsync -avR "${PACKAGE_DIR}"/./* "$uploadDirUnix"
  echo "=== Listing files in upload directory ==="
  ls "$uploadDirUnix"
  # Define the upload filename
  uploadFilename="Mudlet-$VERSION$MUDLET_VERSION_BUILD-$BUILD_COMMIT-windows-$BUILD_BITNESS"

  echo "FOLDER_TO_UPLOAD=${uploadDir}\\" >> "$GITHUB_ENV"
  echo "UPLOAD_FILENAME=$uploadFilename" >> "$GITHUB_ENV"
  # Move packaged files to the upload directory
  #moveToUploadDir "$uploadFilename"
else

  # Check if it's a Public Test Build
  if [[ "$PublicTestBuild" == "true" ]]; then

    # Get the commit date of the last commit
    COMMIT_DATE=$(git show -s --format="%cs")
    # Get yesterday's date in the same format
    YESTERDAY_DATE=$(date --date="yesterday" +%Y-%m-%d)

    if [[ "$COMMIT_DATE" < "$YESTERDAY_DATE" ]]; then
      echo "=== No new commits, aborting public test build generation ==="
      exit 3
    fi

    echo "=== Creating a public test build ==="
    # Squirrel uses Start menu name from the binary, renaming it
    mv "$PACKAGE_DIR/mudlet.exe" "$PACKAGE_DIR/Mudlet PTB.exe"
    # ensure sha part always starts with a character due to a known issue
    VersionAndSha="${VERSION}-ptb-${BUILD_COMMIT}"

  else
    echo "=== Creating a release build ==="
    mv "$PACKAGE_DIR/mudlet.exe" "$PACKAGE_DIR/Mudlet.exe"
    VersionAndSha="$VERSION"
  fi

  echo "=== Cloning installer project ==="
  git clone https://github.com/Mudlet/installers.git "$GITHUB_WORKSPACE/installers"
  cd "$GITHUB_WORKSPACE/installers/windows" || exit 1

  echo "=== Installing Squirrel for Windows ==="
  nuget install squirrel.windows -ExcludeVersion

  echo "=== Setting up directories ==="
  SQUIRRELWIN="$GITHUB_WORKSPACE/squirrel-packaging-prep"
  SQUIRRELWINBIN="$SQUIRRELWIN/lib/net45/"

  if [[ ! -d "$SQUIRRELWINBIN" ]]; then
    mkdir -p "$SQUIRRELWINBIN"
  fi

  echo "=== Moving things to where Squirrel expects them ==="
  mv "$PACKAGE_DIR/"* "$SQUIRRELWINBIN"

  # Set the path to the nuspec file
  NuSpec="$GITHUB_WORKSPACE/installers/windows/mudlet.nuspec"
  echo "=== Creating Nuget package ==="

  # Check if this is a Public Test Build
  if [[ "$PublicTestBuild" == "true" ]]; then
    # Allow public test builds to be installed side by side with the release builds by renaming the app
    # No dots in the <id>: Guidelines by Squirrel
    sed -i "s/<id>Mudlet<\/id>/<id>Mudlet_${BUILD_BITNESS}_-PublicTestBuild<\/id>/" "$NuSpec"
    sed -i "s/<title>Mudlet<\/title>/<title>Mudlet_${BUILD_BITNESS} (Public Test Build)<\/title>/" "$NuSpec"
  else
    sed -i "s/<id>Mudlet<\/id>/<id>Mudlet_${BUILD_BITNESS}_<\/id>/" "$NuSpec"
    sed -i "s/<title>Mudlet<\/title>/<title>Mudlet_${BUILD_BITNESS}<\/title>/" "$NuSpec"
  fi

  # Create NuGet package
  nuget pack "$NuSpec" -Version "$VersionAndSha" -BasePath "$SQUIRRELWIN" -OutputDirectory "$SQUIRRELWIN"

  echo "=== Creating installers from Nuget package ==="
  if [[ "$PublicTestBuild" == "true" ]]; then
    TestBuildString="-PublicTestBuild"
    InstallerIconFile="$GITHUB_WORKSPACE/src/icons/mudlet_ptb.ico"
  else
    TestBuildString=""
    InstallerIconFile="$GITHUB_WORKSPACE/src/icons/mudlet.ico"
  fi

  nupkg_path="$GITHUB_WORKSPACE/squirrel-packaging-prep/Mudlet$TestBuildString.$VersionAndSha.nupkg"
  if [[ ! -f "$nupkg_path" ]]; then
    echo "=== ERROR: nupkg doesn't exist as expected! Build aborted."
    exit 4
  fi

  # Execute Squirrel to create the installer
  ./squirrel.windows/tools/Squirrel --releasify "$nupkg_path" \
    --releaseDir "$GITHUB_WORKSPACE/squirreloutput" \
    --loadingGif "$GITHUB_WORKSPACE/installers/windows/splash-installing-2x.png" \
    --no-msi --setupIcon "$InstallerIconFile" \
    -n "/a /f $GITHUB_WORKSPACE/installers/windows/code-signing-certificate.p12 /p $WIN_SIGNING_PASS /fd sha256 /tr http://timestamp.digicert.com /td sha256"

  echo "=== Removing old directory content of release folder ==="
  rm -rf "${PACKAGE_DIR:?}/*"

  echo "=== Copying installer over ==="
  mv "$GITHUB_WORKSPACE/squirreloutput/*" "$PACKAGE_DIR"

  setupExePath="$PACKAGE_DIR/Setup.exe"

  # Check if the setup executable exists
  if [[ ! -f "$setupExePath" ]]; then
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

  if [[ "$PublicTestBuild" == "true" ]]; then
    echo "=== Uploading public test build to make.mudlet.org ==="
    
    uploadFilename="Mudlet-$VERSION$MUDLET_VERSION_BUILD-$BUILD_COMMIT-windows-$BUILD_BITNESS.exe"
    moveToUploadDir "$uploadFilename"
  else

    echo "=== Uploading installer to https://www.mudlet.org/wp-content/files/?C=M;O=D ==="
    echo "scp -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null $setupExePath 'mudmachine@mudlet.org:${DEPLOY_PATH}'"
    # upload an unzipped, unversioned release for appimage.github.io
    #scp -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "Mudlet.AppImage" "mudmachine@mudlet.org:${DEPLOY_PATH}"
    DEPLOY_URL="https://www.mudlet.org/wp-content/files/Mudlet-${VERSION}-windows-$BUILD_BITNESS-installer.exe"

    SHA256SUM=$(shasum -a 256 "$setupExePath" | awk '{print $1}')

    # file_cat=3 asuming Windows is the 3rd item in WP-Download-Manager category
    curl -X POST 'https://www.mudlet.org/wp-content/plugins/wp-downloadmanager/download-add.php' \
    -H "x-wp-download-token: $X_WP_DOWNLOAD_TOKEN" \
    -F "file_type=2" \
    -F "file_remote=$DEPLOY_URL" \
    -F "file_name=Mudlet-${VERSION} (Linux)" \
    -F "file_des=sha256: $SHA256SUM" \
    -F "file_cat=3" \
    -F "file_permission=-1" \
    -F "output=json" \
    -F "do=Add File"
  fi
  
  echo "=== Installing dblsqd-cli ==="
  npm install -g dblsqd-cli
  dblsqd login -e "https://api.dblsqd.com/v1/jsonrpc" -u "$DBLSQD_USER" -p "$DBLSQD_PASS"

  if [[ "$PublicTestBuild" == "true" ]]; then
    echo "=== Downloading release feed ==="
    DownloadedFeed=$(mktemp)
    curl "https://feeds.dblsqd.com/MKMMR7HNSP65PquQQbiDIw/public-test-build/win/x86" -o "$DownloadedFeed"
    
    echo "=== Generating a changelog ==="
    cd "$GITHUB_WORKSPACE/CI" || exit 1
    Changelog=$(lua "${GITHUB_WORKSPACE}/CI/generate-changelog.lua" --mode ptb --releasefile "$DownloadedFeed")
    cd - || exit 1
    echo "$Changelog"
    
    echo "=== Creating release in Dblsqd ==="
    echo "dblsqd release -a mudlet -c public-test-build-${BUILD_BITNESS} -m \"$Changelog\" \"${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT,,}\""

    echo "=== Registering release with Dblsqd ==="
    echo "dblsqd push -a mudlet -c public-test-build-${BUILD_BITNESS} -r '${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT,,}' -s mudlet --type 'standalone' --attach win:${DBLSQDTYPE} '${DEPLOY_URL}'"
  fi
fi

if [[ -n "$GITHUB_PULL_REQUEST_NUMBER" ]]; then
  prId=" ,#$GITHUB_PULL_REQUEST_NUMBER"
fi

echo ""
echo "******************************************************"
echo ""
if [[ -z "$MUDLET_VERSION_BUILD" ]]; then
  # A release build
  echo "Finished building Mudlet $VERSION"
else
  # Not a release build so include the Git SHA1 in the message
  echo "Finished building Mudlet $VERSION$MUDLET_VERSION_BUILD-$BUILD_COMMIT"
fi

if [[ -n "$DEPLOY_URL" ]]; then
  echo "Deployed the output to $DEPLOY_URL"
fi

echo ""
echo "******************************************************"
