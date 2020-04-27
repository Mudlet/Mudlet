#!/bin/bash

set -e

# we deploy only certain builds
if [ "${DEPLOY}" = "deploy" ]; then

  if [ "$TRAVIS_EVENT_TYPE" = "cron" ]; then
    # instead of deployment, we upload to coverity for cron jobs
    cd build
    tar czf Mudlet.tgz cov-int
    ls -l Mudlet.tgz
    # we make this FAIL to not thrash our allowance if things go wrong!
    curl --form token="${COVERITY_SCAN_TOKEN}" \
      --form email=coverity@mudlet.org \
      --form file=@Mudlet.tgz \
      --form version="master branch head" \
      --form description="$(git log -1|head -1)" \
      --cacert "${HOME}/ca-file.pem" \
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
                   "https://make.mudlet.org/snapshots/Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-linux-x64.AppImage.tar" -O - -q)
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
    # upload an unzipped, unversioned release for appimage.github.io
    scp -i /tmp/mudlet-deploy-key -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "Mudlet.AppImage" "keneanung@mudlet.org:${DEPLOY_PATH}"
    DEPLOY_URL="https://www.mudlet.org/wp-content/files/Mudlet-${VERSION}-linux-x64.AppImage.tar"


    # push release to DBLSQD
    npm install -g dblsqd-cli
    dblsqd login -e "https://api.dblsqd.com/v1/jsonrpc" -u "${DBLSQD_USER}" -p "${DBLSQD_PASS}"
    dblsqd push -a mudlet -c release -r "${VERSION}" -s mudlet --type "standalone" --attach linux:x86_64 "${DEPLOY_URL}"

    # generate and deploy source tarball
    cd "${HOME}"
    # get the archive script
    wget https://raw.githubusercontent.com/meitar/git-archive-all.sh/master/git-archive-all.sh

    cd "${TRAVIS_BUILD_DIR}"
    # generate and upload the tarball
    bash "${HOME}/git-archive-all.sh" "Mudlet-${VERSION}.tar"
    xz "Mudlet-${VERSION}.tar"
    scp -i /tmp/mudlet-deploy-key -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "Mudlet-${VERSION}.tar.xz" "keneanung@mudlet.org:${DEPLOY_PATH}"
    
    # generate and deploy geyser documentation
    luarocks install --local ldoc
    cd "${TRAVIS_BUILD_DIR}/src/mudlet-lua"
    ./genDoc.sh
    scp -i /tmp/mudlet-deploy-key -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -r mudlet-lua-doc/files/ "keneanung@mudlet.org:${GEYSERDOC_DEPLOY_PATH}"
  fi
  export DEPLOY_URL
fi

