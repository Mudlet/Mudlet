#!/bin/bash
if [ -z "${TRAVIS_OS_NAME}" ] || [ "${TRAVIS_OS_NAME}" = "linux" ]; then
  echo Deploy on linux.
  . CI/travis.linux.after_success.sh;
fi
if [ "${TRAVIS_OS_NAME}" = "osx" ]; then
  echo Deploy on OSX.
  . CI/travis.osx.after_success.sh;
fi

echo "******************************************************"
echo ""
echo "Finished building Mudlet ${VERSION}${BUILD}"
if [ ! -z "${DEPLOY_URL}" ]; then
  echo "Deployed the output to ${DEPLOY_URL}"
fi
echo ""
echo "******************************************************"
