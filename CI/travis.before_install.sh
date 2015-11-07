#!/bin/bash
echo Running travis.before_install.sh and TRAVIS_OS_NAME is currently: ${TRAVIS_OS_NAME} ...
if [ -z "${TRAVIS_OS_NAME}" ] || [ "${TRAVIS_OS_NAME}" = "linux" ]; then
  echo Before install on linux.
  ./CI/travis.linux.before_install.sh;
fi
if [ "${TRAVIS_OS_NAME}" = "osx" ]; then
  echo Before install on OSX.
  ./CI/travis.osx.before_install.sh;
fi
