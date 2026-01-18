#!/bin/bash
if [ "${TRAVIS_OS_NAME}" = "osx" ]; then
  echo Before install on OSX.
  ./CI/osx.before_install.sh;
fi
