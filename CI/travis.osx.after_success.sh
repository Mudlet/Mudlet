#!/bin/bash

set -e

# we deploy only qmake and clang combination for macOS
if [ "${Q_OR_C_MAKE}" = "qmake"  -a "${CC}" = "clang" ]; then
  git clone https://github.com/Mudlet/installers.git "${TRAVIS_BUILD_DIR}/../installers"

  cd "${TRAVIS_BUILD_DIR}/../installers/osx"

  ln -s "${TRAVIS_BUILD_DIR}" source

  bash make-installer.sh

  wget --method PUT --body-file="${HOME}/Desktop/Mudlet.dmg" https://transfer.sh/Mudlet.dmg -O - -nv
fi

