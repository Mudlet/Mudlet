#!/bin/bash
if [ -z "${TRAVIS_OS_NAME}" ] || [ "${TRAVIS_OS_NAME}" = "linux" ]; then
  echo Install on linux.
  ./CI/travis.linux.install.sh;
fi
if [ "${TRAVIS_OS_NAME}" = "osx" ]; then
  echo Install on OSX.
  ./CI/travis.osx.install.sh;
fi
