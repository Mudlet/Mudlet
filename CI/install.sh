#!/bin/bash

if [ "${TRAVIS_OS_NAME}" = "osx" ]; then
  echo Install on OSX.
  source CI/osx.install.sh;
fi
if [ "${TRAVIS_OS_NAME}" = "linux" ]; then
  echo Install on linux.
  source CI/linux.install.sh;
fi
if [ ! -z "${CXX}" ]; then
  echo "Testing (possibly updated) compiler version:"
  ${CXX} --version;
fi
