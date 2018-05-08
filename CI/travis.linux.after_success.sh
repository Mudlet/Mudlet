#!/bin/bash

set -e

# we deploy only qmake and gcc combination for linux
if [ "${Q_OR_C_MAKE}" = "qmake" ] && [ "${CC}" = "gcc" ]; then

  if [ "$TRAVIS_EVENT_TYPE" = "cron" ]; then
    # instead of deployment, we upload to coverity for cron jobs
	  tar czf Mudlet.tgz build/cov-int
    ls -l Mudlet.tgz
    # we make this FAIL to not thrash our allowance if things go wrong!
    curl --form token=$COVERITY_SCAN_TOKEN \
      --form email=vperetokin@gmail.com \
      --form file=@Mudlet.tgz \
      --form version="master branch head" \
      --form description="$(git log -1|head -1)" \
      https://scan.coverity.com/builds?project=Mudlet%2FMudlet
    CURL_RESULT=$?
    echo curl returned $CURL_RESULT
    if [ $CURL_RESULT -ne 0 ]; then
	    echo Upload to Coverity failed, curl returned $CURL_RESULT
	    exit 1
    fi
	  exit
  fi

  git clone https://github.com/Mudlet/installers.git "${TRAVIS_BUILD_DIR}/../installers"

  cd "${TRAVIS_BUILD_DIR}/../installers/generic-linux"

  ln -s "${TRAVIS_BUILD_DIR}" source

  # unset LD_LIBRARY_PATH as it upsets linuxdeployqt
  export LD_LIBRARY_PATH=

  if [ -z "${TRAVIS_TAG}" ]; then
    bash make-installer.sh "${VERSION}${MUDLET_VERSION_BUILD}"

    chmod +x "Mudlet-${VERSION}${MUDLET_VERSION_BUILD}.AppImage"

    tar -cvf "Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-linux-x64.AppImage.tar" "Mudlet-${VERSION}${MUDLET_VERSION_BUILD}.AppImage"

    DEPLOY_URL=$(wget --method PUT --body-file="Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-linux-x64.AppImage.tar" \
                   "https://transfer.sh/Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-linux-x64.AppImage.tar" -O - -q)
  else

    # add ssh-key to ssh-agent for deployment
    # shellcheck disable=2154
    # the two "undefined" variables are defined by travis
    openssl aes-256-cbc -K "${encrypted_70dbe4c5e427_key}" -iv "${encrypted_70dbe4c5e427_iv}" -in "${TRAVIS_BUILD_DIR}/CI/mudlet-deploy-key.enc" -out /tmp/mudlet-deploy-key -d
    eval "$(ssh-agent -s)"
    chmod 600 /tmp/mudlet-deploy-key
    ssh-add /tmp/mudlet-deploy-key

    bash make-installer.sh -r "${VERSION}"

    chmod +x "Mudlet.AppImage"

    tar -czvf "Mudlet-${VERSION}-linux-x64.AppImage.tar" "Mudlet.AppImage"

    scp -i /tmp/mudlet-deploy-key -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "Mudlet-${VERSION}-linux-x64.AppImage.tar" "keneanung@mudlet.org:${DEPLOY_PATH}"
    DEPLOY_URL="http://www.mudlet.org/wp-content/files/Mudlet-${VERSION}-linux-x64.AppImage.tar"
  fi
  export DEPLOY_URL
fi

