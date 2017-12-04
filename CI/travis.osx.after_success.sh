#!/bin/bash

set -e

# we deploy only qmake and clang combination for macOS
if [ "${Q_OR_C_MAKE}" = "qmake" ] && [ "${CC}" = "clang" ]; then
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
    security set-key-partition-list -S apple-tool:,apple: -s -k "$CERT_PW" "$KEYCHAIN"
    export IDENTITY="Developer ID Application"
    echo "Imported identity:"
    security find-identity
    echo "----"
  fi

  ln -s "${TRAVIS_BUILD_DIR}" source

  if [ -z "${TRAVIS_TAG}" ]; then # PR build
    appBaseName="Mudlet-${VERSION}${MUDLET_VERSION_BUILD}"
    mv "source/build/Mudlet.app" "source/build/${appBaseName}.app"

    bash make-installer.sh "${appBaseName}.app"

    if [ ! -z "$CERT_PW" ]; then
      codesign --deep -s "$IDENTITY" "${HOME}/Desktop/${appBaseName}.dmg"
      echo "Signed final .dmg"
    fi

    DEPLOY_URL=$(wget --method PUT --body-file="${HOME}/Desktop/${appBaseName}.dmg"  "https://transfer.sh/${appBaseName}.dmg" -O - -q)
  else # release build

    # add ssh-key to ssh-agent for deployment
    # shellcheck disable=2154
    # the two "undefined" variables are defined by travis
    openssl aes-256-cbc -K "${encrypted_70dbe4c5e427_key}" -iv "${encrypted_70dbe4c5e427_iv}" -in "${TRAVIS_BUILD_DIR}/CI/mudlet-deploy-key.enc" -out /tmp/mudlet-deploy-key -d
    eval "$(ssh-agent -s)"
    chmod 600 /tmp/mudlet-deploy-key
    ssh-add /tmp/mudlet-deploy-key

    bash make-installer.sh -r "${VERSION}" source/build/Mudlet.app

    if [ ! -z "$CERT_PW" ]; then
      codesign --deep -s "$IDENTITY" "${HOME}/Desktop/Mudlet.dmg"
      echo "Signed final .dmg"
    fi

    mv "${HOME}/Desktop/Mudlet.dmg" "${HOME}/Desktop/Mudlet-${VERSION}.dmg"

    scp -i /tmp/mudlet-deploy-key -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "${HOME}/Desktop/Mudlet-${VERSION}.dmg" "keneanung@mudlet.org:${DEPLOY_PATH}"
    DEPLOY_URL="http://www.mudlet.org/wp-content/files/Mudlet-${VERSION}.dmg"
  fi

  # delete keychain just in case
  if [ ! -z "$CERT_PW" ]; then
    security delete-keychain $KEYCHAIN
  fi

  export DEPLOY_URL
fi

