#!/bin/bash

set -e

if [[ "${MUDLET_VERSION_BUILD}" == -ptb* ]]; then
  public_test_build="true"
fi

# we deploy only certain builds
if [ "${DEPLOY}" = "deploy" ]; then
  git clone https://github.com/Mudlet/installers.git "${TRAVIS_BUILD_DIR}/../installers"

  cd "${TRAVIS_BUILD_DIR}/../installers/osx"

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

  ln -s "${TRAVIS_BUILD_DIR}" source

  if [ -z "${TRAVIS_TAG}" ] && [ "${public_test_build}" != "true" ]; then # PR build
    echo "== Creating a snapshot build =="
    appBaseName="Mudlet-${VERSION}${MUDLET_VERSION_BUILD}"
    mv "source/build/Mudlet.app" "source/build/${appBaseName}.app"

    bash make-installer.sh "${appBaseName}.app"

    if [ ! -z "$CERT_PW" ]; then
      codesign --deep -s "$IDENTITY" "${HOME}/Desktop/${appBaseName}.dmg"
      echo "Signed final .dmg"
    fi

    DEPLOY_URL=$(wget --method PUT --body-file="${HOME}/Desktop/${appBaseName}.dmg"  "https://make.mudlet.org/snapshots/${appBaseName}.dmg" -O - -q)

  else # ptb/release build
    app="${TRAVIS_BUILD_DIR}/build/Mudlet.app"
    if [ "${public_test_build}" == "true" ]; then
      echo "== Creating a public test build =="
      mv "$app" "source/build/Mudlet PTB.app"
      app="source/build/Mudlet PTB.app"
    else
      echo "== Creating a release build =="
    fi

    # add ssh-key to ssh-agent for deployment
    # shellcheck disable=2154
    # the two "undefined" variables are defined by travis
    if [ "${public_test_build}" != "true" ]; then
      echo "=== Registering Mudlet SSH keys for release upload ==="
      openssl aes-256-cbc -K "${encrypted_70dbe4c5e427_key}" -iv "${encrypted_70dbe4c5e427_iv}" -in "${TRAVIS_BUILD_DIR}/CI/mudlet-deploy-key.enc" -out /tmp/mudlet-deploy-key -d
      eval "$(ssh-agent -s)"
      chmod 600 /tmp/mudlet-deploy-key
      ssh-add /tmp/mudlet-deploy-key
    fi

    if [ "${public_test_build}" == "true" ]; then
      bash make-installer.sh -pr "${VERSION}${MUDLET_VERSION_BUILD}" "$app"
    else
      bash make-installer.sh -r "${VERSION}" "$app"
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
      echo "=== Uploading public test build to make.mudlet.org ==="
      DEPLOY_URL=$(wget --method PUT --body-file="${HOME}/Desktop/Mudlet-${VERSION}${MUDLET_VERSION_BUILD}.dmg"  "https://make.mudlet.org/snapshots/Mudlet-${VERSION}${MUDLET_VERSION_BUILD}.dmg" -O - -q)
    else
      echo "=== Uploading installer to https://www.mudlet.org/wp-content/files/?C=M;O=D ==="
      scp -i /tmp/mudlet-deploy-key -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "${HOME}/Desktop/Mudlet-${VERSION}.dmg" "keneanung@mudlet.org:${DEPLOY_PATH}"
      DEPLOY_URL="https://www.mudlet.org/wp-content/files/Mudlet-${VERSION}.dmg"
    fi

    # install dblsqd. NPM must be available here because we use it to install the tool that creates the dmg
    npm install -g dblsqd-cli
    dblsqd login -e "https://api.dblsqd.com/v1/jsonrpc" -u "${DBLSQD_USER}" -p "${DBLSQD_PASS}"

    if [ "${public_test_build}" == "true" ]; then
      echo "=== Creating release in Dblsqd ==="
      dblsqd release -a mudlet -c public-test-build -m "(test release message here)" "${VERSION}${MUDLET_VERSION_BUILD}" || true

      echo "=== Registering release with Dblsqd ==="
      dblsqd push -a mudlet -c public-test-build -r "${VERSION}${MUDLET_VERSION_BUILD}" -s mudlet --type "standalone" --attach mac:x86_64 "${DEPLOY_URL}" || true
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

