#!/bin/bash

set -x

if { [ -n "$TRAVIS_REPO_SLUG" ] && [ "${TRAVIS_REPO_SLUG}" != "Mudlet/Mudlet" ]; } ||
   { [ -n "$GITHUB_REPOSITORY" ] && [ "${GITHUB_REPOSITORY}" != "Mudlet/Mudlet" ]; } then
  exit 0
fi

if [ "${TRAVIS_OS_NAME}" = "linux" ] || [ "${RUNNER_OS}" = "Linux" ]; then
  echo Deploy on Linux.
  . CI/travis.linux.after_success.sh;
  echo $?
  echo "^ worked?"
elif [ "${TRAVIS_OS_NAME}" = "osx" ]  || [ "${RUNNER_OS}" = "macOS" ]; then
  echo Deploy on macOS.
  . CI/travis.osx.after_success.sh;
fi

if [ "${TRAVIS_PULL_REQUEST}" != "false" ]; then
  prId=" ,#${TRAVIS_PULL_REQUEST}"
fi

if [ ! -z "${DEPLOY_URL}" ]; then
  curl \
    --data-urlencode "message=Deployed Mudlet \`${VERSION}${MUDLET_VERSION_BUILD}\` (${TRAVIS_OS_NAME}${prId}) to [${DEPLOY_URL}](${DEPLOY_URL})" \
    https://webhooks.gitter.im/e/cc99072d43b642c4673a
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
