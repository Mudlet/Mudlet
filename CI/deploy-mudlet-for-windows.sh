#!/bin/bash
cd "$GITHUB_WORKSPACE" || exit

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
  exit 0
fi

cd "$GITHUB_WORKSPACE/package-MINGW64-release" || exit

moveToUploadDir() {
  local uploadFilename=$1
  echo "=== Setting up upload directory ==="
  local uploadDir="$GITHUB_WORKSPACE/upload"

  # Check if the upload directory exists, if not, create it
  if [[ ! -d "$uploadDir" ]]; then
    mkdir -p "$uploadDir"
  fi

  echo "=== Moving files to upload directory ==="
  mv "$GITHUB_WORKSPACE/package-MINGW64-release/$uploadFilename" "$uploadDir/"

  # Append these variables to the GITHUB_ENV to make them available in subsequent steps
  echo "FOLDER_TO_UPLOAD=$uploadDir" >> "$GITHUB_ENV"
  echo "UPLOAD_FILENAME=$uploadFilename" >> "$GITHUB_ENV"
}


# Remove specific file types from the directory
rm ./*.cpp ./*.o


# Check if GITHUB_REPO_TAG is "false" and PublicTestBuild is not true
if [[ "$GITHUB_REPO_TAG" == "false" ]] && [[ "$PublicTestBuild" == false ]]; then
  echo "=== Creating a snapshot build ==="
  mv "$GITHUB_WORKSPACE/package-MINGW64-release/mudlet.exe" "Mudlet.exe"
  
  # Create a zip file using 7z
  7z a "Mudlet-$VERSION$MUDLET_VERSION_BUILD-$BUILD_COMMIT-windows.zip" "$GITHUB_WORKSPACE/package-MINGW64-release/*"
  
  # Define the upload filename
  uploadFilename="Mudlet-$VERSION$MUDLET_VERSION_BUILD-$BUILD_COMMIT-windows.zip"
  
  # Move the zip file to the upload directory (assumed to be previously defined as a function or script)
  moveToUploadDir "$uploadFilename"
else

  # Check if it's a Public Test Build
  if [[ "$PublicTestBuild" == "true" ]]; then

    # Get the commit date of the last commit
    COMMIT_DATE=$(git show -s --format="%cs")
    # Get yesterday's date in the same format
    YESTERDAY_DATE=$(date --date="yesterday" +%Y-%m-%d)

    if [[ "$COMMIT_DATE" < "$YESTERDAY_DATE" ]]; then
      echo "=== No new commits, aborting public test build generation ==="
      exit 0
    fi

    echo "=== Creating a public test build ==="
    # Squirrel uses Start menu name from the binary, renaming it
    mv "$GITHUB_WORKSPACE/package-MINGW64-release/mudlet.exe" "$GITHUB_WORKSPACE/package-MINGW64-release/Mudlet PTB.exe"
    # ensure sha part always starts with a character due to a known issue
    VersionAndSha="${VERSION}-ptb-${BUILD_COMMIT}"

  else
    echo "=== Creating a release build ==="
    mv "$GITHUB_WORKSPACE/package-MINGW64-release/mudlet.exe" "$GITHUB_WORKSPACE/package-MINGW64-release/Mudlet.exe"
    VersionAndSha="$VERSION"
  fi

  echo "=== Cloning installer project ==="
  git clone https://github.com/Mudlet/installers.git "$GITHUB_WORKSPACE/installers"
  cd "$GITHUB_WORKSPACE/installers/windows" || exit

  echo "=== Installing Squirrel for Windows ==="
  nuget install squirrel.windows -ExcludeVersion

  echo "=== Setting up directories ==="
  SQUIRRELWIN="$GITHUB_WORKSPACE/squirrel-packaging-prep"
  SQUIRRELWINBIN="$SQUIRRELWIN/lib/net45/"

  if [[ ! -d "$SQUIRRELWINBIN" ]]; then
    mkdir -p "$SQUIRRELWINBIN"
  fi

  echo "=== Moving things to where Squirrel expects them ==="
  mv "$GITHUB_WORKSPACE/package-MINGW64-release/"* "$SQUIRRELWINBIN"

  # Set the path to the nuspec file
  NuSpec="$GITHUB_WORKSPACE/installers/windows/mudlet.nuspec"
  echo "=== Creating Nuget package ==="

  # Check if this is a Public Test Build
  if [[ "$PublicTestBuild" == "true" ]]; then
    # Allow public test builds to be installed side by side with the release builds by renaming the app
    # No dots in the <id>: Guidelines by Squirrel
    sed -i 's/<id>Mudlet<\/id>/<id>Mudlet-PublicTestBuild<\/id>/' "$NuSpec"
    sed -i 's/<title>Mudlet<\/title>/<title>Mudlet (Public Test Build)<\/title>/' "$NuSpec"
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
    exit 1
  fi

  # Execute Squirrel to create the installer
  ./squirrel.windows/tools/Squirrel --releasify "$nupkg_path" \
    --releaseDir "$GITHUB_WORKSPACE/squirreloutput" \
    --loadingGif "$GITHUB_WORKSPACE/installers/windows/splash-installing-2x.png" \
    --no-msi --setupIcon "$InstallerIconFile" \
    -n "/a /f $GITHUB_WORKSPACE/installers/windows/code-signing-certificate.p12 /p $WIN_SIGNING_PASS /fd sha256 /tr http://timestamp.digicert.com /td sha256"

  echo "=== Removing old directory content of release folder ==="
  rm -rf "$GITHUB_WORKSPACE/package-MINGW64-release/*"

  echo "=== Copying installer over ==="
  mv "$GITHUB_WORKSPACE/squirreloutput/*" "$GITHUB_WORKSPACE/package-MINGW64-release"

  setupExePath="$GITHUB_WORKSPACE/package-MINGW64-release/Setup.exe"

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

    exit 1
  fi

  if [[ "$PublicTestBuild" == "true" ]]; then
    echo "=== Uploading public test build to make.mudlet.org ==="
    
    uploadFilename="Mudlet-$VERSION$MUDLET_VERSION_BUILD-$BUILD_COMMIT-windows.exe"
    moveToUploadDir "$uploadFilename"
  else

    echo "=== Uploading installer to https://www.mudlet.org/wp-content/files/?C=M;O=D ==="
    echo "scp -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null $setupExePath 'mudmachine@mudlet.org:${DEPLOY_PATH}'"
    # upload an unzipped, unversioned release for appimage.github.io
    #scp -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "Mudlet.AppImage" "mudmachine@mudlet.org:${DEPLOY_PATH}"
    DEPLOY_URL="https://www.mudlet.org/wp-content/files/Mudlet-${VERSION}-windows-installer.exe"

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
    
    echo "=== Installing dblsqd-cli ==="
    npm install -g dblsqd-cli
    dblsqd login -e "https://api.dblsqd.com/v1/jsonrpc" -u "$DBLSQD_USER" -p "$DBLSQD_PASS"

    if [[ "$PublicTestBuild" == "true" ]]; then
      echo "=== Downloading release feed ==="
      DownloadedFeed=$(mktemp)
      curl "https://feeds.dblsqd.com/MKMMR7HNSP65PquQQbiDIw/public-test-build/win/x86" -o "$DownloadedFeed"
    
      echo "=== Generating a changelog ==="
      cd "$GITHUB_WORKSPACE/CI" || exit
      Changelog=$(lua "${GITHUB_WORKSPACE}/CI/generate-changelog.lua" --mode ptb --releasefile "$DownloadedFeed")
      cd - || exit
      echo "$Changelog"
    
      echo "=== Creating release in Dblsqd ==="
      echo "dblsqd release -a mudlet -c public-test-build -m \"$Changelog\" \"${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT,,}\""

      echo "=== Registering release with Dblsqd ==="
     echo "dblsqd push -a mudlet -c public-test-build -r '${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT,,}' -s mudlet --type 'standalone' --attach win:x86 '${DEPLOY_URL}'"
   fi
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


