#!/bin/bash
if [ -z "${TRAVIS_OS_NAME}" ] || [ "${TRAVIS_OS_NAME}" = "linux" ]; then
  echo Deploy on linux.
  bash CI/travis.linux.after_success.sh;
fi
if [ "${TRAVIS_OS_NAME}" = "osx" ]; then
  echo Deploy on OSX.
  bash CI/travis.osx.after_success.sh;
fi
