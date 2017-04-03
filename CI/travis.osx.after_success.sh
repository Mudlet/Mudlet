#!/bin/bash

set -e

# we deploy only qmake and clang combination for macOS
if [ "${Q_OR_C_MAKE}" = "qmake"  -a "${CC}" = "clang" ]; then
  git clone https://github.com/Mudlet/installers.git "${TRAVIS_BUILD_DIR}/../installers"

  cd "${TRAVIS_BUILD_DIR}/../installers/osx"

  ln -s "${TRAVIS_BUILD_DIR}" source

  if [ -z "${TRAVIS_TAG}" ]; then
    appBaseName="Mudlet-${VERSION}${BUILD}"
    mv "source/build/Mudlet.app" "source/build/${appBaseName}.app"

    bash make-installer.sh "${appBaseName}.app"

    export DEPLOY_URL=$(wget --method PUT --body-file="${HOME}/Desktop/${appBaseName}.dmg"  https://transfer.sh/${appBaseName}.dmg -O - -q)
  else

    # add ssh-key to ssh-agent for deployment
    openssl aes-256-cbc -K $encrypted_70dbe4c5e427_key -iv $encrypted_70dbe4c5e427_iv -in "${TRAVIS_BUILD_DIR}/CI/mudlet-deploy-key.enc" -out /tmp/mudlet-deploy-key -d
    eval "$(ssh-agent -s)"
    chmod 600 /tmp/mudlet-deploy-key
    ssh-add /tmp/mudlet-deploy-key

    bash make-installer.sh -r "${VERSION}" source/build/Mudlet.app

    mv "${HOME}/Desktop/Mudlet.dmg" "${HOME}/Desktop/Mudlet-${VERSION}.dmg"

    export DEPLOY_URL=$(scp -i /tmp/mudlet-deploy-key "${HOME}/Desktop/Mudlet-${VERSION}.dmg" "keneanung@mudlet.org:${DEPLOY_PATH}")
  fi
fi

