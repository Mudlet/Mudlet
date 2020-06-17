#!/bin/bash

set -e

if [[ "${MUDLET_VERSION_BUILD}" == -ptb* ]]; then
  public_test_build="true"
fi

# we deploy only linux+deploy or cron+clang+cmake for PTB
if { [ "${TRAVIS_OS_NAME}" = "linux" ] && [ "${DEPLOY}" = "deploy" ]; } ||
   { [ "${TRAVIS_EVENT_TYPE}" = "cron" ] && [ "${TRAVIS_OS_NAME}" = "linux" ] &&  [ "${CC}" = "clang" ] && [ "${Q_OR_C_MAKE}" = "cmake" ]; } then

  if [ "$TRAVIS_EVENT_TYPE" = "cron" ] && [ "${TRAVIS_OS_NAME}" = "linux" ] && [ "${DEPLOY}" = "deploy" ]; then
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

  if [ -z "${TRAVIS_TAG}" ] && [ "${public_test_build}" != "true" ]; then
    echo "== Creating a snapshot build =="
    bash make-installer.sh "${VERSION}${MUDLET_VERSION_BUILD}"

    chmod +x "Mudlet-${VERSION}${MUDLET_VERSION_BUILD}.AppImage"

    tar -cvf "Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-linux-x64.AppImage.tar" "Mudlet-${VERSION}${MUDLET_VERSION_BUILD}.AppImage"

    DEPLOY_URL=$(wget --method PUT --body-file="Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-linux-x64.AppImage.tar" \
                   "https://make.mudlet.org/snapshots/Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-linux-x64.AppImage.tar" -O - -q)
  else # ptb/release build
    if [ "${public_test_build}" == "true" ]; then
      echo "== Creating a public test build =="
    else
      echo "== Creating a release build =="
    fi

    # add ssh-key to ssh-agent for deployment
    # shellcheck disable=2154
    # the two "undefined" variables are defined by travis
    if [ "${public_test_build}" != "true" ]; then
      echo "=== Registering Mudlet SSH keys for release upload ==="
      openssl aes-256-cbc -K "${encrypted_70dbe4c5e427_key}" -iv "${encrypted_70dbe4c5e427_iv}" -in "${TRAVIS_BUILD_DIR}/CI/mudlet-deploy-key.enc" -out /tmp/mudlet-deploy-key -d
      eval "$(ssh-agent -s)"
      chmod 600 /tmp/mudlet-deploy-key
      ssh-add /tmp/mudlet-deploy-key
    fi

    if [ "${public_test_build}" == "true" ]; then
      bash make-installer.sh -pr "${VERSION}${MUDLET_VERSION_BUILD}"
    else
      bash make-installer.sh -r "${VERSION}"
    fi

    if [ "${public_test_build}" == "true" ]; then
      chmod +x "Mudlet PTB.AppImage"
    else
      chmod +x "Mudlet.AppImage"
    fi

    if [ "${public_test_build}" == "true" ]; then
      tar -czvf "Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-linux-x64.AppImage.tar" "Mudlet PTB.AppImage"
    else
      tar -czvf "Mudlet-${VERSION}-linux-x64.AppImage.tar" "Mudlet.AppImage"
    fi

    if [ "${public_test_build}" == "true" ]; then
      echo "=== Uploading public test build to make.mudlet.org ==="
      DEPLOY_URL=$(wget --method PUT --body-file="Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-linux-x64.AppImage.tar" \
                     "https://make.mudlet.org/snapshots/Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-linux-x64.AppImage.tar" -O - -q)
    else
      echo "=== Uploading installer to https://www.mudlet.org/wp-content/files/?C=M;O=D ==="
      scp -i /tmp/mudlet-deploy-key -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "Mudlet-${VERSION}-linux-x64.AppImage.tar" "keneanung@mudlet.org:${DEPLOY_PATH}"
      # upload an unzipped, unversioned release for appimage.github.io
      scp -i /tmp/mudlet-deploy-key -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "Mudlet.AppImage" "keneanung@mudlet.org:${DEPLOY_PATH}"
      DEPLOY_URL="https://www.mudlet.org/wp-content/files/Mudlet-${VERSION}-linux-x64.AppImage.tar"
    fi

    # push release to DBLSQD
    npm install -g dblsqd-cli
    dblsqd login -e "https://api.dblsqd.com/v1/jsonrpc" -u "${DBLSQD_USER}" -p "${DBLSQD_PASS}"

    if [ "${public_test_build}" == "true" ]; then
      echo "=== Downloading release feed ==="
      downloadedfeed=$(mktemp)
      wget "https://feeds.dblsqd.com/MKMMR7HNSP65PquQQbiDIw/public-test-build/linux/x86_64" --output-document="$downloadedfeed"
      echo "=== Generating a changelog ==="
      cd "${TRAVIS_BUILD_DIR}"
      changelog=$(lua "${TRAVIS_BUILD_DIR}/CI/generate-ptb-changelog.lua" --releasefile "${downloadedfeed}")

      echo "=== Creating release in Dblsqd ==="
      dblsqd release -a mudlet -c public-test-build -m "${changelog}" "${VERSION}${MUDLET_VERSION_BUILD}" || true

      echo "=== Registering release with Dblsqd ==="
      dblsqd push -a mudlet -c public-test-build -r "${VERSION}${MUDLET_VERSION_BUILD}" -s mudlet --type "standalone" --attach linux:x86_64 "${DEPLOY_URL}" || true
    else
      echo "=== Registering release with Dblsqd ==="
      dblsqd push -a mudlet -c release -r "${VERSION}" -s mudlet --type "standalone" --attach linux:x86_64 "${DEPLOY_URL}"
    fi

    if [ "${public_test_build}" != "true" ]; then
      # generate and deploy source tarball
      cd "${HOME}"
      # get the archive script
      wget https://raw.githubusercontent.com/meitar/git-archive-all.sh/master/git-archive-all.sh

      cd "${TRAVIS_BUILD_DIR}"
      # generate and upload the tarball
      bash "${HOME}/git-archive-all.sh" "Mudlet-${VERSION}.tar"
      xz "Mudlet-${VERSION}.tar"
      scp -i /tmp/mudlet-deploy-key -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "Mudlet-${VERSION}.tar.xz" "keneanung@mudlet.org:${DEPLOY_PATH}"
    fi
  fi
  export DEPLOY_URL
fi

