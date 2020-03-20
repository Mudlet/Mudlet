#!/bin/bash

if [ -z "${TRAVIS_TAG}" ]; then
  exit 0
fi

VALID_QMAKE=$(cat src/mudlet.pro | pcregrep --only-matching=1 "^VERSION ? = ?(\d+\.\d+\.\d+)$")

if [ -z "${VALID_QMAKE}" ]; then
  echo "mudlet.pro isn't set to a valid version - aborting build."
  exit 1
fi

VALID_CMAKE=$(cat CMakeLists.txt | pcregrep --only-matching=1 "set\(APP_VERSION (\d+\.\d+\.\d+)\)$")

if [ -z "${VALID_CMAKE}" ]; then
  echo "CMakeLists.txt isn't set to a valid version - aborting build."
  exit 1
fi
