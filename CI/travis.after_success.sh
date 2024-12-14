#!/bin/bash

if { [ -n "$GITHUB_REPOSITORY" ] && [ "${GITHUB_REPOSITORY}" != "Mudlet/Mudlet" ]; } then
  exit 0
fi

if [ "${RUNNER_OS}" = "Linux" ]; then
  echo Deploy on Linux.
  . CI/travis.linux.after_success.sh;
elif [ "${RUNNER_OS}" = "macOS" ]; then
  echo Deploy on macOS.
  . CI/travis.osx.after_success.sh;
fi

echo ""
echo "******************************************************"
echo ""
if [ -z ${MUDLET_VERSION_BUILD} ]; then
  # A release build
  echo "Finished building Mudlet ${VERSION}"
else
  # Not a release build so include the details including the Git SHA1 in the message
  echo "Finished building Mudlet ${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}"
fi
if [ ! -z "${DEPLOY_URL}" ]; then
  echo "Deployed the output to ${DEPLOY_URL}"
fi
echo ""
echo "******************************************************"
