#!/bin/bash

set -e

sign_and_notarize () {

  local appBundle="$1"
  codesign --deep -o runtime -s "$IDENTITY" "${appBundle}"
  echo "Signed final .dmg"

  for i in {1..3}; do
    echo "Trying to notarize (attempt ${i})"
    if xcrun notarytool submit "${appBundle}" --apple-id "$APPLE_USERNAME" --password "$APPLE_PASSWORD" --team-id "$APPLE_TEAM_ID" --wait; then
      break
    fi
  done
}

BUILD_DIR="${BUILD_FOLDER}"
SOURCE_DIR="${GITHUB_WORKSPACE}"

if [[ "${MUDLET_VERSION_BUILD}" == -ptb* ]]; then
  public_test_build="true"
fi

if [ "${DEPLOY}" = "deploy" ]; then

  # get commit date now before we check out an change into another git repository
  COMMIT_DATE=$(git show -s --pretty="tformat:%cI" | cut -d'T' -f1 | tr -d '-')
  YESTERDAY_DATE=$(date -v-1d '+%F' | tr -d '-')

  git clone https://github.com/Mudlet/installers.git "${BUILD_DIR}/../installers"

  cd "${BUILD_DIR}/../installers/osx"

  # setup macOS keychain for code signing on ptb/release builds.
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

  if ! [[ "$GITHUB_REF" =~ ^"refs/tags/" ]] && [ "${public_test_build}" != "true" ]; then
    echo "== Creating a snapshot build =="
    appBaseName="Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-${ARCH}"
    if [ -n "${GITHUB_REPOSITORY}" ]; then
      mv "${BUILD_DIR}/src/mudlet.app" "${BUILD_DIR}/${appBaseName}.app"
    else
      mv "${BUILD_DIR}/Mudlet.app" "${BUILD_DIR}/${appBaseName}.app"
    fi

    ./make-installer.sh "${appBaseName}.app"

    if [ -n "$MACOS_SIGNING_PASS" ]; then
      sign_and_notarize "${HOME}/Desktop/${appBaseName}.dmg"
    fi

    echo "=== ... later, via Github ==="
    # Move the finished file into a folder of its own, because we ask Github to upload contents of a folder
    mkdir -p "${BUILD_DIR}/upload/"
    mv "${HOME}/Desktop/${appBaseName}.dmg" "${BUILD_DIR}/upload/"
    {
      echo "FOLDER_TO_UPLOAD=${BUILD_DIR}/upload"
      echo "UPLOAD_FILENAME=${appBaseName}"
    } >> "$GITHUB_ENV"
    DEPLOY_URL="Github artifact, see https://github.com/$GITHUB_REPOSITORY/actions/runs/$GITHUB_RUN_ID"
  else # ptb/release build
    app="${BUILD_DIR}/build/mudlet.app"
    if [ "${public_test_build}" == "true" ]; then

      if [[ "${COMMIT_DATE}" -lt "${YESTERDAY_DATE}" ]]; then
        echo "== No new commits, aborting public test build generation =="
        exit 0
      fi

      echo "== Creating a public test build =="
      if [ -n "${GITHUB_REPOSITORY}" ]; then
        mv "${BUILD_DIR}/src/mudlet.app" "${BUILD_DIR}/Mudlet PTB.app"
      else
        mv "$app" "source/build/Mudlet PTB.app"
      fi

      app="${BUILD_DIR}/Mudlet PTB.app"
    else
      echo "== Creating a release build =="
    fi

    if [ "${public_test_build}" == "true" ]; then
      ./make-installer.sh -pr "${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-${ARCH}" "$app"
    else
      ./make-installer.sh -r "${VERSION}-${ARCH}" "$app"
    fi

    if [ ! -z "$MACOS_SIGNING_PASS" ]; then
      if [ "${public_test_build}" == "true" ]; then
        sign_and_notarize "${HOME}/Desktop/Mudlet PTB.dmg"
      else
        sign_and_notarize "${HOME}/Desktop/Mudlet.dmg"
      fi
    fi

    if [ "${public_test_build}" == "true" ]; then
      mv "${HOME}/Desktop/Mudlet PTB.dmg" "${HOME}/Desktop/Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-${ARCH}.dmg"
    else
      mv "${HOME}/Desktop/Mudlet.dmg" "${HOME}/Desktop/Mudlet-${VERSION}-${ARCH}.dmg"
    fi

    if [ "${public_test_build}" == "true" ]; then
      echo "=== Setting up for Github upload ==="
      mkdir -p "${BUILD_DIR}/upload/"
      mv "${HOME}/Desktop/Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-${ARCH}.dmg" "${BUILD_DIR}/upload/"
      {
        echo "FOLDER_TO_UPLOAD=${BUILD_DIR}/upload"
        echo "UPLOAD_FILENAME=Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-${ARCH}"
      } >> "$GITHUB_ENV"
      DEPLOY_URL="Github artifact, see https://github.com/$GITHUB_REPOSITORY/actions/runs/$GITHUB_RUN_ID"
    else
      echo "=== Uploading installer to https://www.mudlet.org/wp-content/files/?C=M;O=D ==="
      scp -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "${HOME}/Desktop/Mudlet-${VERSION}-${ARCH}.dmg" "mudmachine@mudlet.org:${DEPLOY_PATH}"
      DEPLOY_URL="https://www.mudlet.org/wp-content/files/Mudlet-${VERSION}-${ARCH}.dmg"

      if ! curl --output /dev/null --silent --head --fail "$DEPLOY_URL"; then
        echo "Error: release not found as expected at $DEPLOY_URL"
        exit 1
      fi

      SHA256SUM=$(shasum -a 256 "${HOME}/Desktop/Mudlet-${VERSION}-${ARCH}.dmg" | awk '{print $1}')

      if [ "${ARCH}" = "arm64" ]; then
        FILE_CATEGORY="4"
      else
        FILE_CATEGORY="3"
      fi

      # Get current timestamp
      current_timestamp=$(date "+%-d %-m %Y %-H %-M %-S")
      read -r day month year hour minute second <<< "$current_timestamp"

      curl --retry 5 -X POST 'https://www.mudlet.org/download-add.php' \
      -H "x-wp-download-token: $X_WP_DOWNLOAD_TOKEN" \
      -F "file_type=2" \
      -F "file_remote=$DEPLOY_URL" \
      -F "file_name=Mudlet ${VERSION}-${ARCH} (macOS)" \
      -F "file_des=sha256: $SHA256SUM" \
      -F "file_cat=${FILE_CATEGORY}" \
      -F "file_permission=-1" \
      -F "file_timestamp_day=$day" \
      -F "file_timestamp_month=$month" \
      -F "file_timestamp_year=$year" \
      -F "file_timestamp_hour=$hour" \
      -F "file_timestamp_minute=$minute" \
      -F "file_timestamp_second=$second" \
      -F "output=json" \
      -F "do=Add File"

    fi

    # install dblsqd. NPM must be available here because we use it to install the tool that creates the dmg
    npm install -g dblsqd-cli
    dblsqd login -e "https://api.dblsqd.com/v1/jsonrpc" -u "${DBLSQD_USER}" -p "${DBLSQD_PASS}"

    if [ "${public_test_build}" == "true" ]; then
      echo "=== Downloading release feed ==="
      downloadedfeed=$(mktemp)
      wget "https://feeds.dblsqd.com/MKMMR7HNSP65PquQQbiDIw/public-test-build/mac/${ARCH_DBLSQD}" --output-document="$downloadedfeed"
      echo "=== Generating a changelog ==="
      cd "${SOURCE_DIR}" || exit
      changelog=$(lua "${SOURCE_DIR}/CI/generate-changelog.lua" --mode ptb --releasefile "${downloadedfeed}")

      echo "=== Creating release in Dblsqd ==="
      dblsqd release -a mudlet -c public-test-build -m "${changelog}" "${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-${ARCH}" || true

      # release registration and uploading will be manual for the time being
    else
      echo "=== Registering release with Dblsqd ==="
      dblsqd push -a mudlet -c release -r "${VERSION}" -s mudlet --type "standalone" --attach mac:${ARCH_DBLSQD} "${DEPLOY_URL}"
    fi
  fi

  # delete keychain just in case
  if [ ! -z "$MACOS_SIGNING_PASS" ]; then
    security delete-keychain $KEYCHAIN
  fi

  export DEPLOY_URL
fi
