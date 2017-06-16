#!/bin/bash
if [ "${TRAVIS_OS_NAME}" = "osx" ]; then
  echo Install on OSX.
  ./CI/travis.osx.install.sh;
fi
if [ "${TRAVIS_OS_NAME}" = "linux" ]; then
  # add GCC 5 to path
  mkdir -p latest-gcc-symlinks
  ln -s /usr/bin/g++-5 latest-gcc-symlinks/g++
  ln -s /usr/bin/gcc-5 latest-gcc-symlinks/gcc
  export PATH=$PWD/latest-gcc-symlinks:$PATH
fi
if [ ! -z "${CXX}" ]; then
  echo "Testing (possibly updated) compiler version:"
  ${CXX} --version;
fi
