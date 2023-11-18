#!/bin/bash

set -e

sign_and_notarize () {

  local appBundle="$1"
  codesign --deep -o runtime -s "$IDENTITY" "${appBundle}"
  echo "Signed final .dmg"

  cat << EOF > gon.json
{
  "notarize": [{
    "path": "${appBundle}",
    "bundle_id": "mudlet",
    "staple": true
  }]
}
EOF

  for i in {1..3}; do
    echo "Trying to notarize (attempt ${i})"
    if gon gon.json; then
      break
    fi
  done

}

BUILD_DIR="${BUILD_FOLDER}"
SOURCE_DIR="${GITHUB_WORKSPACE}"

if [[ "${MUDLET_VERSION_BUILD}" == -ptb* ]]; then
  PUBLIC_TEST_BUILD="true"
fi

# we deploy only certain builds
if [ "${DEPLOY}" = "deploy" ]; then

  git clone https://github.com/SlySven/installers.git -b infrastructure_fix_MacOS_libzstd_1_dylib_crashes "${BUILD_DIR}/../installers"

  cd "${BUILD_DIR}/../installers/osx"

  # setup macOS keychain for code signing on development builds only,
  # as CI's don't allow signing on usual PR builds
  if [ -n "$MACOS_SIGNING_PASS" ]; then
    KEYCHAIN=build.keychain
    security create-keychain -p travis $KEYCHAIN
    security default-keychain -s $KEYCHAIN
    security unlock-keychain -p travis $KEYCHAIN
    security set-keychain-settings -t 3600 -u $KEYCHAIN
    security import Certificates.p12 -k $KEYCHAIN -P "$MACOS_SIGNING_PASS" -T /usr/bin/codesign
    security set-key-partition-list -S apple-tool:,apple: -s -k travis $KEYCHAIN
    export IDENTITY="Developer ID Application"
    echo "Imported identity:"
    security find-identity
    echo "----"
  fi

  if ! [[ "$GITHUB_REF" =~ ^"refs/tags/" ]] && [ "${PUBLIC_TEST_BUILD}" != "true" ]; then
    # No need to repeat this
    # echo "== Creating a snapshot build =="
    APP_BASE_NAME="Mudlet-${VERSION}${MUDLET_VERSION_BUILD}"

    if [ -n "$MACOS_SIGNING_PASS" ]; then
      sign_and_notarize "${HOME}/Desktop/${APP_BASE_NAME}.dmg"
    fi

    echo "=== ... later, via Github ==="
    # Move the finished file into a folder of its own, because we ask Github to upload contents of a folder
    mkdir "upload/"
    mv "${HOME}/Desktop/${APP_BASE_NAME}.dmg" "upload/"
    {
      echo "FOLDER_TO_UPLOAD=$(pwd)/upload"
      echo "UPLOAD_FILENAME=${APP_BASE_NAME}"
    } >> "$GITHUB_ENV"
    DEPLOY_URL="Github artifact, see https://github.com/${GITHUB_REPOSITORY}/runs/${GITHUB_RUN_ID}"

  else # ptb/release build
    if [ "${PUBLIC_TEST_BUILD}" == "true" ]; then
      if [ -f ${BUILD_DIR}/ptb_unchanged.txt ]; then
        # No need to repeat this
        # echo "== No new commits, aborting public test build generation =="
        exit 0
      fi

      if [ ! -f ${BUILD_DIR}/ptb_changed.txt ]; then
        echo "== Error, PTB build is missing a ${BUILD_DIR}/ptb_changed.txt or ${BUILD_DIR}/ptb_unchanged.txt file, aborting public test build generation =="
        exit 0
      fi

      # No need to repeat this
      #echo "== Creating a public test build =="

      if [ ! -z "$MACOS_SIGNING_PASS" ]; then
        sign_and_notarize "${HOME}/Desktop/Mudlet PTB.dmg"
      fi

      # Give the PTB a unique name
      mv "${HOME}/Desktop/Mudlet PTB.dmg" "${HOME}/Desktop/Mudlet-${VERSION}${MUDLET_VERSION_BUILD}.dmg"

      echo "=== Setting up for Github upload ==="
      mkdir "upload/"
      mv "${HOME}/Desktop/Mudlet-${VERSION}${MUDLET_VERSION_BUILD}.dmg" "upload/"
      {
        echo "FOLDER_TO_UPLOAD=$(pwd)/upload"
        echo "UPLOAD_FILENAME=Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-macos"
      } >> "$GITHUB_ENV"
      DEPLOY_URL="Github artifact, see https://github.com/$GITHUB_REPOSITORY/runs/$GITHUB_RUN_ID"

      # install dblsqd. NPM must be available here because we use it to install the tool that creates the dmg
      npm install -g dblsqd-cli
      dblsqd login -e "https://api.dblsqd.com/v1/jsonrpc" -u "${DBLSQD_USER}" -p "${DBLSQD_PASS}"

      echo "=== Downloading release feed ==="
      downloadedfeed=$(mktemp)
      wget "https://feeds.dblsqd.com/MKMMR7HNSP65PquQQbiDIw/public-test-build/mac/x86_64" --output-document="$downloadedfeed"
      echo "=== Generating a changelog ==="
      cd "${SOURCE_DIR}" || exit
      changelog=$(lua "${SOURCE_DIR}/CI/generate-changelog.lua" --mode ptb --releasefile "${downloadedfeed}")

      echo "=== Creating release in Dblsqd ==="
      dblsqd release -a mudlet -c public-test-build -m "${changelog}" "${VERSION}${MUDLET_VERSION_BUILD}" || true

      # release registration and uploading will be manual for the time being
    else
      # No need to repeat this
      # echo "== Creating a release build =="

      if [ ! -z "$MACOS_SIGNING_PASS" ]; then
        sign_and_notarize "${HOME}/Desktop/Mudlet.dmg"
      fi

      # Give the release a unique name
      mv "${HOME}/Desktop/Mudlet.dmg" "${HOME}/Desktop/Mudlet-${VERSION}.dmg"

      echo "=== Uploading installer to https://www.mudlet.org/wp-content/files/?C=M;O=D ==="
      scp -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "${HOME}/Desktop/Mudlet-${VERSION}.dmg" "mudmachine@mudlet.org:${DEPLOY_PATH}"
      DEPLOY_URL="https://www.mudlet.org/wp-content/files/Mudlet-${VERSION}.dmg"

      # install dblsqd. NPM must be available here because we use it to install the tool that creates the dmg
      npm install -g dblsqd-cli
      dblsqd login -e "https://api.dblsqd.com/v1/jsonrpc" -u "${DBLSQD_USER}" -p "${DBLSQD_PASS}"

      echo "=== Registering release with Dblsqd ==="
      dblsqd push -a mudlet -c release -r "${VERSION}" -s mudlet --type "standalone" --attach mac:x86_64 "${DEPLOY_URL}"
    fi
  fi

  # delete keychain just in case
  if [ ! -z "$MACOS_SIGNING_PASS" ]; then
    security delete-keychain $KEYCHAIN
  fi

  export DEPLOY_URL
fi
