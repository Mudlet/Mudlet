#!/bin/bash

# set -e
set -x

BUILD_DIR="${BUILD_FOLDER}"
SOURCE_DIR="${GITHUB_WORKSPACE}"

if [[ "${MUDLET_VERSION_BUILD}" == -ptb* ]]; then
  PUBLIC_TEST_BUILD="true"
fi

# we deploy only if told to deploy or we run a cron+clang+cmake job (for PTB)
if { [ "${DEPLOY}" = "deploy" ]; } \
#   || { [ "${TRAVIS_EVENT_TYPE}" = "cron" ] &&  [ "${CC}" = "clang" ] && [ "${Q_OR_C_MAKE}" = "cmake" ]; }
then

# coverity is currently defunct as it was dependent on our travis infrastructure. See #4887
#  if [ "${TRAVIS_EVENT_TYPE}" = "cron" ] && [ "${DEPLOY}" = "deploy" ]; then
#    # instead of deployment, we upload to coverity for cron jobs
#    cd build
#    tar czf Mudlet.tgz cov-int
#    ls -l Mudlet.tgz
#    # we make this FAIL to not thrash our allowance if things go wrong!
#    curl --form token="${COVERITY_SCAN_TOKEN}" \
#      --form email=coverity@mudlet.org \
#      --form file=@Mudlet.tgz \
#      --form version="master branch head" \
#      --form description="$(git log -1|head -1)" \
#      --cacert "${HOME}/ca-file.pem" \
#      https://scan.coverity.com/builds?project=Mudlet%2FMudlet
#    CURL_RESULT=$?
#    echo curl returned $CURL_RESULT
#    if [ $CURL_RESULT -ne 0 ]; then
#      echo Upload to Coverity failed, curl returned $CURL_RESULT
#      exit 1
#    fi
#    exit 0
#  fi

  # We refer to $BUILD_COMMIT in the environment to get the commit data now
  COMMIT_DATE=$(git show -s --format="%cs" | tr -d '-')
  YESTERDAY_DATE=$(date -d "yesterday" '+%F' | tr -d '-')

  # TODO: revert to master branch once https://github.com/Mudlet/installers/pull/123 is merged:
  git clone --branch Improve_switchTo_linuxdeploy https://github.com/SlySven/installers.git "${BUILD_DIR}/../installers"

  cd "${BUILD_DIR}/../installers/generic-linux" || exit 1

  ln -s "${BUILD_DIR}" source

  # unset LD_LIBRARY_PATH as it upsets linuxdeployqt - CHECK whether linuxdeploy is also upset?
  # export LD_LIBRARY_PATH=

  # The linuxdeploy-qt pluging to linuxdeploy needs to know the Qt version it
  # is being run for when it is run inside the ./make-installer.sh script.
  # This is established from the QMAKE environment variable - see:
  # https://github.com/linuxdeploy/linuxdeploy-plugin-qt
  # The (currently unused) installer repository file
  # ${BUILD_DIR}/../installers/generic-linux/build-and-make-installer.sh sets
  # this before it calls
  # ${BUILD_DIR}/../installers/generic-linux/make-installer.sh
  if [ -z "${QMAKE}" ]; then
    QMAKE=$(which qmake6)
    export QMAKE
  fi

  # Debug: print out the locations of all copies of the buult mudlet file
  echo "Checking for the mudlet file we should have:
  # Currently we seem to be looking in:
  # /home/runner/work/Mudlet/b/ninja/mudlet
  # but that doesn't seem to be correct now...
  find /home/runner/work -name mudlet -type f -print0 | xargs -0 ls -lh

  # fail quickly now:
  exit 1

  if ! [[ "${GITHUB_REF}" =~ ^"refs/tags/" ]] && [ "${PUBLIC_TEST_BUILD}" != "true" ]; then
    echo "== Creating a snapshot build =="
    ./make-installer.sh "${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}"
    cd "${BUILD_DIR}/../installers/generic-linux" || exit 1

    chmod +x "Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}.AppImage"
    tar -cvf "Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-linux-x64.AppImage.tar" "Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}.AppImage"


    echo "=== ... later, via Github ==="
    # Move the finished file into a folder of its own, because we ask Github to upload contents of a folder
    mkdir "upload/"
    mv "Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-linux-x64.AppImage.tar" "upload/"
    {
      echo "FOLDER_TO_UPLOAD=$(pwd)/upload"
      echo "UPLOAD_FILENAME=Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-linux-x64"
    } >> "${GITHUB_ENV}"
    DEPLOY_URL="Github artifact, see https://github.com/${GITHUB_REPOSITORY}/runs/${GITHUB_RUN_ID}"

  else # ptb/release build
    if [ "${PUBLIC_TEST_BUILD}" == "true" ]; then

      if [[ "${COMMIT_DATE}" -lt "${YESTERDAY_DATE}" ]]; then
        echo "== No new commits, aborting public test build generation =="
        exit 0
      fi

      echo "== Creating a public test build =="
    else
      echo "== Creating a release build =="
    fi

    if [ "${PUBLIC_TEST_BUILD}" == "true" ]; then
      ./make-installer.sh -pr "${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}"
    else
      ./make-installer.sh -r "${VERSION}"
    fi

    if [ "${PUBLIC_TEST_BUILD}" == "true" ]; then
      chmod +x "Mudlet PTB.AppImage"
    else
      chmod +x "Mudlet.AppImage"
    fi

    if [ "${PUBLIC_TEST_BUILD}" == "true" ]; then
      tar -cvf "Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-linux-x64.AppImage.tar" "Mudlet PTB.AppImage"
    else
      tar -cvf "Mudlet-${VERSION}-linux-x64.AppImage.tar" "Mudlet.AppImage"
    fi

    if [ "${PUBLIC_TEST_BUILD}" == "true" ]; then
      echo "=== Setting up for Github upload ==="
      mkdir "upload/"
      mv "Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-linux-x64.AppImage.tar" "upload/"
      {
        echo "FOLDER_TO_UPLOAD=$(pwd)/upload"
        echo "UPLOAD_FILENAME=Mudlet-$VERSION$MUDLET_VERSION_BUILD-${BUILD_COMMIT}-linux-x64"
      } >> "${GITHUB_ENV}"
      DEPLOY_URL="Github artifact, see https://github.com/$GITHUB_REPOSITORY/runs/$GITHUB_RUN_ID"
    else
      echo "=== Uploading installer to https://www.mudlet.org/wp-content/files/?C=M;O=D ==="
      scp -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "Mudlet-${VERSION}-linux-x64.AppImage.tar" "mudmachine@mudlet.org:${DEPLOY_PATH}"

      DEPLOY_URL="https://www.mudlet.org/wp-content/files/Mudlet-${VERSION}-linux-x64.AppImage.tar"
      if ! curl --output /dev/null --silent --head --fail "${DEPLOY_URL}"; then
        echo "Error: release not found as expected at ${DEPLOY_URL}"
        exit 1
      fi

      # upload an unzipped, unversioned release for appimage.github.io
      scp -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "Mudlet.AppImage" "mudmachine@mudlet.org:${DEPLOY_PATH}"
      DEPLOY_URL="https://www.mudlet.org/wp-content/files/Mudlet-${VERSION}-linux-x64.AppImage.tar"

      SHA256SUM=$(shasum -a 256 "Mudlet-${VERSION}-linux-x64.AppImage.tar" | awk '{print $1}')
      CURRENT_TIMESTAMP=$(date "+%-d %-m %Y %-H %-M %-S")
      read -r DAY MONTH YEAR HOUR MINUTE SECOND <<< "${CURRENT_TIMESTAMP}"

      curl --retry 5 -X POST 'https://www.mudlet.org/download-add.php' \
      -H "x-wp-download-token: ${X_WP_DOWNLOAD_TOKEN}" \
      -F "file_type=2" \
      -F "file_remote=${DEPLOY_URL}" \
      -F "file_name=Mudlet ${VERSION} (Linux)" \
      -F "file_des=sha256: ${SHA256SUM}" \
      -F "file_cat=5" \
      -F "file_permission=-1" \
      -F "file_timestamp_day=${DAY}" \
      -F "file_timestamp_month=${MONTH}" \
      -F "file_timestamp_year=${YEAR}" \
      -F "file_timestamp_hour=${HOUR}" \
      -F "file_timestamp_minute=${MINUTE}" \
      -F "file_timestamp_second=${SECOND}" \
      -F "output=json" \
      -F "do=Add File"
    fi

    # push release to DBLSQD
    sudo npm install -g dblsqd-cli
    dblsqd login -e "https://api.dblsqd.com/v1/jsonrpc" -u "${DBLSQD_USER}" -p "${DBLSQD_PASS}"

    if [ "${PUBLIC_TEST_BUILD}" == "true" ]; then
      echo "=== Downloading release feed ==="
      DONWLOADFEED=$(mktemp)
      wget "https://feeds.dblsqd.com/MKMMR7HNSP65PquQQbiDIw/public-test-build/linux/x86_64" --output-document="${DONWLOADFEED}"
      echo "=== Generating a changelog ==="
      cd "${SOURCE_DIR}" || exit 1
      CHANGELONG=$(lua "${SOURCE_DIR}/CI/generate-changelog.lua" --mode ptb --releasefile "${DONWLOADFEED}")

      echo "=== Creating release in Dblsqd ==="
      dblsqd release -a mudlet -c public-test-build -m "${CHANGELONG}" "${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}" || true

      # release registration and uploading will be manual for the time being
    else
      echo "=== Registering release with Dblsqd ==="
      dblsqd push -a mudlet -c release -r "${VERSION}" -s mudlet --type "standalone" --attach linux:x86_64 "${DEPLOY_URL}"
    fi

    if [ "${PUBLIC_TEST_BUILD}" != "true" ]; then
      # generate and deploy source tarball
      cd "${HOME}" || exit 1
      # get the archive script
      wget https://raw.githubusercontent.com/meitar/git-archive-all.sh/master/git-archive-all.sh

      cd "${SOURCE_DIR}" || exit 1
      # generate and upload the tarball
      chmod +x "${HOME}/git-archive-all.sh"
      "${HOME}/git-archive-all.sh" "Mudlet-${VERSION}.tar"
      xz "Mudlet-${VERSION}.tar"
      scp -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "Mudlet-${VERSION}.tar.xz" "mudmachine@mudlet.org:${DEPLOY_PATH}"
      FILE_URL="https://www.mudlet.org/wp-content/files/Mudlet-${VERSION}.tar.xz"
      SHA256SUM=$(shasum -a 256 "Mudlet-${VERSION}.tar.xz" | awk '{print $1}')
      CURRENT_TIMESTAMP=$(date "+%-d %-m %Y %-H %-M %-S")
      read -r DAY MONTH YEAR HOUR MINUTE SECOND <<< "${CURRENT_TIMESTAMP}"

      curl --retry 5 -X POST 'https://www.mudlet.org/download-add.php' \
      -H "x-wp-download-token: $X_WP_DOWNLOAD_TOKEN" \
      -F "file_type=2" \
      -F "file_remote=$FILE_URL" \
      -F "file_name=Mudlet ${VERSION} (Source Code)" \
      -F "file_des=sha256: $SHA256SUM" \
      -F "file_cat=6" \
      -F "file_permission=-1" \
      -F "file_timestamp_day=${DAY}" \
      -F "file_timestamp_month=${MONTH}" \
      -F "file_timestamp_year=${YEAR}" \
      -F "file_timestamp_hour=${HOUR}" \
      -F "file_timestamp_minute=${MINUTE}" \
      -F "file_timestamp_second=${SECOND}" \
      -F "output=json" \
      -F "do=Add File"
    fi
  fi
  export DEPLOY_URL
fi
