#!/bin/bash

set -e

# we deploy only qmake and clang combination for macOS
if [ "${Q_OR_C_MAKE}" = "qmake"  -a "${CC}" = "clang" ]; then
  git clone https://github.com/Mudlet/installers.git "${TRAVIS_BUILD_DIR}/../installers"

  cd "${TRAVIS_BUILD_DIR}/../installers/osx"

  ln -s "${TRAVIS_BUILD_DIR}" source

  appBaseName="Mudlet-${VERSION}${BUILD}"
  mv "source/build/Mudlet.app" "source/build/${appBaseName}.app"

  bash make-installer.sh "${appBaseName}.app"

  export DEPLOY_URL=$(wget --method PUT --body-file="${HOME}/Desktop/${appBaseName}.dmg"  https://transfer.sh/${appBaseName}.dmg -O - -q)
fi

