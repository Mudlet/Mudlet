#!/bin/bash

set -e

git clone https://github.com/Mudlet/installers.git "${TRAVIS_BUILD_DIR}/../installers"

cd "${TRAVIS_BUILD_DIR}/../installers/osx"

ln -s "${TRAVIS_BUILD_DIR}" source

bash make-installer.sh
