#!/bin/bash

set -e

[ -n "$TRAVIS_REPO_SLUG" ] && BUILD_DIR="${TRAVIS_BUILD_DIR}" || BUILD_DIR="${BUILD_FOLDER}"
[ -n "$TRAVIS_REPO_SLUG" ] && SOURCE_DIR="${TRAVIS_BUILD_DIR}" || SOURCE_DIR="${GITHUB_WORKSPACE}"

if [[ "${MUDLET_VERSION_BUILD}" == -ptb* ]]; then
  public_test_build="true"
fi

# we deploy only certain builds
if [ "${DEPLOY}" = "deploy" ]; then

  # get commit date now before we check out an change into another git repository
  COMMIT_DATE=$(git show -s --pretty="tformat:%cI" | cut -d'T' -f1 | tr -d '-')
  YESTERDAY_DATE=$(date -v-1d '+%F' | tr -d '-')

  git clone https://github.com/Mudlet/installers.git "${BUILD_DIR}/../installers"

  cd "${BUILD_DIR}/../installers/osx"

  # setup macOS keychain for code signing on development builds only,
  # as Travis does not allow signing on usual PR builds
  if [ ! -z "$CERT_PW" ]; then
    KEYCHAIN=build.keychain
    security create-keychain -p travis $KEYCHAIN
    security default-keychain -s $KEYCHAIN
    security unlock-keychain -p travis $KEYCHAIN
    security set-keychain-settings -t 3600 -u $KEYCHAIN
    security import Certificates.p12 -k $KEYCHAIN -P "$CERT_PW" -T /usr/bin/codesign
    OSX_VERSION=$(sw_vers -productVersion | cut -d '.' -f 1,2)
    if [ "${OSX_VERSION}" != "10.11" ]; then
      # This is a new command on 10.12 and above, so don't run it on 10.11 (lowest supported version)
      security set-key-partition-list -S apple-tool:,apple: -s -k travis $KEYCHAIN
    fi
    export IDENTITY="Developer ID Application"
    echo "Imported identity:"
    security find-identity
    echo "----"
  fi

  ln -s "${BUILD_DIR}" source

  if [ -z "${TRAVIS_TAG}" ] && ! [[ "$GITHUB_REF" =~ ^"refs/tags/" ]] && [ "${public_test_build}" != "true" ]; then
    echo "== Creating a snapshot build =="
    appBaseName="Mudlet-${VERSION}${MUDLET_VERSION_BUILD}"
    if [ -n "${GITHUB_REPOSITORY}" ]; then
      mv "${BUILD_DIR}/src/mudlet.app" "${BUILD_DIR}/${appBaseName}.app"
    else
      mv "${BUILD_DIR}/Mudlet.app" "${BUILD_DIR}/${appBaseName}.app"
    fi

    ./make-installer.sh "${appBaseName}.app"

    if [ ! -z "$CERT_PW" ]; then
      codesign --deep -s "$IDENTITY" "${HOME}/Desktop/${appBaseName}.dmg"
      echo "Signed final .dmg"
    fi

    if [ -n "$TRAVIS_REPO_SLUG" ]; then
      DEPLOY_URL=$(wget --method PUT --body-file="${HOME}/Desktop/${appBaseName}.dmg"  "https://make.mudlet.org/snapshots/${appBaseName}.dmg" -O - -q)
    else
      echo "=== ... later, via Github ==="
      # Move the finished file into a folder of its own, because we ask Github to upload contents of a folder
      mkdir "upload/"
      mv "${HOME}/Desktop/${appBaseName}.dmg" "upload/"
      {
        echo "FOLDER_TO_UPLOAD=$(pwd)/upload"
        echo "UPLOAD_FILENAME=${appBaseName}"
      } >> "$GITHUB_ENV"
      DEPLOY_URL="Github artifact, see https://github.com/$GITHUB_REPOSITORY/runs/$GITHUB_RUN_ID"
    fi
  else # ptb/release build
    app="${BUILD_DIR}/build/Mudlet.app"
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
      ./make-installer.sh -pr "${VERSION}${MUDLET_VERSION_BUILD}" "$app"
    else
      ./make-installer.sh -r "${VERSION}" "$app"
    fi

    if [ ! -z "$CERT_PW" ]; then
      if [ "${public_test_build}" == "true" ]; then
        codesign --deep -s "$IDENTITY" "${HOME}/Desktop/Mudlet PTB.dmg"
      else
        codesign --deep -s "$IDENTITY" "${HOME}/Desktop/Mudlet.dmg"
      fi
      echo "Signed final .dmg"
    fi

    if [ "${public_test_build}" == "true" ]; then
      mv "${HOME}/Desktop/Mudlet PTB.dmg" "${HOME}/Desktop/Mudlet-${VERSION}${MUDLET_VERSION_BUILD}.dmg"
    else
      mv "${HOME}/Desktop/Mudlet.dmg" "${HOME}/Desktop/Mudlet-${VERSION}.dmg"
    fi

    if [ "${public_test_build}" == "true" ]; then
      echo "=== Setting up for Github upload ==="
      mkdir "upload/"
      mv "${HOME}/Desktop/Mudlet-${VERSION}${MUDLET_VERSION_BUILD}.dmg" "upload/"
      {
        echo "FOLDER_TO_UPLOAD=$(pwd)/upload"
        echo "UPLOAD_FILENAME=Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-macos"
      } >> "$GITHUB_ENV"
      DEPLOY_URL="Github artifact, see https://github.com/$GITHUB_REPOSITORY/runs/$GITHUB_RUN_ID"
    else
      echo "=== Uploading installer to https://www.mudlet.org/wp-content/files/?C=M;O=D ==="
      scp -i "${BUILD_DIR}/CI/mudlet-deploy-key-github.decoded" -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "${HOME}/Desktop/Mudlet-${VERSION}.dmg" "keneanung@mudlet.org:${DEPLOY_PATH}"
      DEPLOY_URL="https://www.mudlet.org/wp-content/files/Mudlet-${VERSION}.dmg"
    fi

    # install dblsqd. NPM must be available here because we use it to install the tool that creates the dmg
    npm install -g dblsqd-cli
    dblsqd login -e "https://api.dblsqd.com/v1/jsonrpc" -u "${DBLSQD_USER}" -p "${DBLSQD_PASS}"

    if [ "${public_test_build}" == "true" ]; then
      echo "=== Downloading release feed ==="
      downloadedfeed=$(mktemp)
      wget "https://feeds.dblsqd.com/MKMMR7HNSP65PquQQbiDIw/public-test-build/mac/x86_64" --output-document="$downloadedfeed"
      echo "=== Generating a changelog ==="
      cd "${SOURCE_DIR}" || exit
      changelog=$(lua "${SOURCE_DIR}/CI/generate-ptb-changelog.lua" --releasefile "${downloadedfeed}")

      echo "=== Creating release in Dblsqd ==="
      dblsqd release -a mudlet -c public-test-build -m "${changelog}" "${VERSION}${MUDLET_VERSION_BUILD}" || true

      # release registration and uploading will be manual for the time being
    else
      echo "=== Registering release with Dblsqd ==="
      dblsqd push -a mudlet -c release -r "${VERSION}" -s mudlet --type "standalone" --attach mac:x86_64 "${DEPLOY_URL}"
    fi
  fi

  # delete keychain just in case
  if [ ! -z "$CERT_PW" ]; then
    security delete-keychain $KEYCHAIN
  fi

  export DEPLOY_URL
fi
