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

# Release tags have to spell out APP_VERSION - CI/check-release-tag.sh explains
# what goes wrong when they do not
function validate_release_tag() {
  local TAG_NAME=""
  if [[ "${GITHUB_REF:-}" =~ ^refs/tags/ ]]; then
    TAG_NAME="${GITHUB_REF#refs/tags/}"
  fi
  if [ -z "${TAG_NAME}" ]; then
    error "This is a release build, but the tag being built could not be determined from GITHUB_REF."
  fi

  local APP_VERSION
  APP_VERSION=$(pcre2grep --only-matching=1 "set\(APP_VERSION (.+)\)$" < CMakeLists.txt)
  if [ -z "${APP_VERSION}" ]; then
    error "No set(APP_VERSION ...) line could be read out of CMakeLists.txt."
  fi

  bash "$(dirname "${BASH_SOURCE[0]}")/check-release-tag.sh" "${APP_VERSION}" "${TAG_NAME}" || exit $?
}

function validate_updater_environment_variable() {
  if [ "$WITH_UPDATER" == "NO" ]; then
    error "Updater is disabled in a release build."
  fi
}

validate_release_tag
validate_cmake
validate_updater_environment_variable

