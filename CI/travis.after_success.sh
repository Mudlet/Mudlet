#!/bin/bash

#Used when debugging, print each expanded command before it is executed:
#set -x

if { [ -n "$GITHUB_REPOSITORY" ] && [ "${GITHUB_REPOSITORY}" != "Mudlet/Mudlet" ]; } then
  exit 0
fi

if [ "${RUNNER_OS}" = "Linux" ]; then
  echo Deploy on Linux.
  . CI/travis.linux.after_success.sh;
elif [ "${RUNNER_OS}" = "macOS" ]; then
  echo Deploy on macOS.
  if [ -z "${DEPLOY_PART}" ]; then
    # not set so do all parts
    . CI/travis.osx.after_success_part1.sh;
    . CI/travis.osx.after_success_part2.sh;
  elif [ ${DEPLOY_PART} -eq 1 ]; then
    # only do the first part`
    . CI/travis.osx.after_success_part1.sh;
  elif [ ${DEPLOY_PART} -eq 2 ]; then
    # only do the second part`
    . CI/travis.osx.after_success_part2.sh;
  else
    echo "Invalid DEPLOY_PART value, should only be 1 or 2"
    # And fall back to doing both bits
    . CI/travis.osx.after_success_part1.sh;
    . CI/travis.osx.after_success_part2.sh;
  fi
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
