#!/bin/bash

set -x

if { [ -n "$GITHUB_REPOSITORY" ] && [ "${GITHUB_REPOSITORY}" != "Mudlet/Mudlet" ]; } then
  exit 0
fi

if [ "${RUNNER_OS}" = "Linux" ]; then
  echo Deploy on Linux.
  . CI/travis.linux.after_success.sh;
  echo $?
  echo "^ worked?"
elif [ "${RUNNER_OS}" = "macOS" ]; then
  echo Deploy on macOS.
  . CI/travis.osx.after_success.sh;
fi

echo ""
echo "******************************************************"
echo ""
echo "Finished building Mudlet ${VERSION}${MUDLET_VERSION_BUILD}"
if [ ! -z "${DEPLOY_URL}" ]; then
  echo "Deployed the output to ${DEPLOY_URL}"
fi
echo ""
echo "******************************************************"
