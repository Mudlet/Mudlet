#!/bin/bash

if [[ "$GITHUB_REPO_TAG" == "false" ]]; then
  echo "Not a release build - skipping release validation."
  exit 0
fi

error() {
  # shellcheck disable=SC2059
  printf "error: $1\n" "${@:2}" >&2
  exit 1
}

function validate_cmake() {
  local VALID_CMAKE
  VALID_CMAKE=$(pcre2grep --only-matching=1 "set\(APP_VERSION (\d+\.\d+\.\d+)\)$" < CMakeLists.txt)

  if [ -z "${VALID_CMAKE}" ]; then
    error "CMakeLists.txt VERSION variable isn't formatted following the semantic versioning rules in a release build."
  fi

  # The build suffix is no longer hardcoded in CMakeLists.txt: tag builds get a
  # clean version automatically because set-build-info.sh exports an empty
  # MUDLET_VERSION_BUILD, which CMakeLists.txt turns into an empty APP_BUILD. So
  # there is no static "APP_BUILD" line left to validate here.
}

function validate_updater_environment_variable() {
  if [ "$WITH_UPDATER" == "NO" ]; then
    error "Updater is disabled in a release build."
  fi
}

validate_cmake
validate_updater_environment_variable

