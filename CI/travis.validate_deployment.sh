#!/bin/bash

if [ -z "${TRAVIS_TAG}" ]; then
  echo "Not a release build - skipping release validation."
  exit 0
fi

error() {
  # shellcheck disable=SC2059
  printf "error: $1\n" "${@:2}" >&2
  exit 1
}

function validate_qmake() {
  local VALID_QMAKE VALID_BUILD

  VALID_QMAKE=$(pcregrep --only-matching=1 "^VERSION ? = ?(\d+\.\d+\.\d+)$" < src/mudlet.pro)
  if [ -z "${VALID_QMAKE}" ]; then
    error "mudlet.pro's VERSION variable isn't formatted following the semantic versioning rules in a release build."
  fi

  VALID_BUILD=$(pcregrep --only-matching=1 ' +BUILD ? = ? ("")' < src/mudlet.pro)
  if [ "${VALID_BUILD}" != '""' ]; then
    error "mudlet.pro's BUILD variable isn't set to \"\" as it should be in a release build."
  fi
}

function validate_cmake() {
  local VALID_CMAKE VALID_BUILD
  VALID_CMAKE=$(pcregrep --only-matching=1 "set\(APP_VERSION (\d+\.\d+\.\d+)\)$" < CMakeLists.txt)

  if [ -z "${VALID_CMAKE}" ]; then
    error "CMakeLists.txt VERSION variable isn't formatted following the semantic versioning rules in a release build."
  fi

  VALID_BUILD=$(pcregrep --only-matching=1 'set\(APP_BUILD ("")\)$' < CMakeLists.txt)
  if [ "${VALID_BUILD}" != '""' ]; then
    error "CMakeLists.txt APP_BUILD variable isn't set to \"\" as it should be in a release build."
  fi
}

function validate_updater_environment_variable() {
  if [ "$WITH_UPDATER" == "NO" ]; then
     error "Updater is disabled in a release build."
  fi
}

validate_qmake
validate_cmake
validate_updater_environment_variable
