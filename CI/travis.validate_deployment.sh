#!/bin/bash

if [ -z "${TRAVIS_TAG}" ]; then
  exit 0
fi

function validate_qmake() {
  VALID_QMAKE=$(cat src/mudlet.pro | pcregrep --only-matching=1 "^VERSION ? = ?(\d+\.\d+\.\d+)$")

  if [ -z "${VALID_QMAKE}" ]; then
  echo "mudlet.pro isn't set to a valid version in a release build."
  exit 1
  fi
}

function validate_cmake() {
  VALID_CMAKE=$(cat CMakeLists.txt | pcregrep --only-matching=1 "set\(APP_VERSION (\d+\.\d+\.\d+)\)$")

  if [ -z "${VALID_CMAKE}" ]; then
    echo "CMakeLists.txt isn't set to a valid version in a release build."
    exit 1
  fi
}

function validate_updater_environment_variable() {
  if [ "$WITH_UPDATER" != "NO" ]; then
     echo "Updater is disabled in a release build."
     exit 1
  fi
}

validate_qmake
validate_cmake
validate_updater_environment_variable
