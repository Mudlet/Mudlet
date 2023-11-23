#!/bin/bash

set -e

BUILD_DIR="${BUILD_FOLDER}"
SOURCE_DIR="${GITHUB_WORKSPACE}"

if [[ "${MUDLET_VERSION_BUILD}" == -ptb* ]]; then
  PUBLIC_TEST_BUILD="true"
fi

# we deploy only certain builds
if [ "${DEPLOY}" = "deploy" ]; then

  # get commit date now before we check out a change into another git repository
  COMMIT_DATE=$(git show -s --pretty="tformat:%cI" | cut -d'T' -f1 | tr -d '-')
  YESTERDAY_DATE=$(date -v-1d '+%F' | tr -d '-')

  git clone https://github.com/SlySven/installers.git -b Infrastructure_update_liblzma_version "${BUILD_DIR}/../installers"

  cd "${BUILD_DIR}/../installers/osx"

  if ! [[ "$GITHUB_REF" =~ ^"refs/tags/" ]] && [ "${PUBLIC_TEST_BUILD}" != "true" ]; then
    echo "== Creating a snapshot build =="
    APP_BASE_NAME="Mudlet-${VERSION}${MUDLET_VERSION_BUILD}"
    if [ -n "${GITHUB_REPOSITORY}" ]; then
      mv "${BUILD_DIR}/src/mudlet.app" "${BUILD_DIR}/${APP_BASE_NAME}.app"
    else
      mv "${BUILD_DIR}/Mudlet.app" "${BUILD_DIR}/${APP_BASE_NAME}.app"
    fi

    # This script is provided by the separate Mudlet "installers" repository
    ./make-installer.sh "${APP_BASE_NAME}.app"

  else # ptb/release build
    if [ "${PUBLIC_TEST_BUILD}" == "true" ]; then

      # Create one of two sentinel files we check in part2
      if [[ "${COMMIT_DATE}" -lt "${YESTERDAY_DATE}" ]]; then
        touch ${BUILD_DIR}/ptb_unchanged.txt
        if [ -f ${BUILD_DIR}/ptb_changed.txt ]; then
          # And remove the opposite one if it is there
          rm ${BUILD_DIR}/ptb_changed.txt
        fi
        echo "== No new commits, aborting public test build generation =="
        exit 0
      else
        touch ${BUILD_DIR}/ptb_changed.txt
        if [ -f ${BUILD_DIR}/ptb_unchanged.txt ]; then
          # And remove the opposite one if it is there
          rm ${BUILD_DIR}/ptb_unchanged.txt
        fi
      fi

      echo "== Creating a public test build =="
      if [ -n "${GITHUB_REPOSITORY}" ]; then
        mv "${BUILD_DIR}/src/mudlet.app" "${BUILD_DIR}/Mudlet PTB.app"
      else
        # The destination directory does look suspect as it may not be
        # compatible with the next step - if that actually uses the third
        # argument - though it doesn't seem to currently!
        mv "${BUILD_DIR}/build/Mudlet.app" "source/build/Mudlet PTB.app"
      fi

      # This script is provided by the separate Mudlet "installers" repository
      ./make-installer.sh -pr "${VERSION}${MUDLET_VERSION_BUILD}" "${BUILD_DIR}/Mudlet PTB.app"

    else
      echo "== Creating a release build =="

      # This script is provided by the separate Mudlet "installers" repository
      ./make-installer.sh -r "${VERSION}" "${BUILD_DIR}/build/Mudlet.app"
    fi
  fi

fi
