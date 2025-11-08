#!/bin/bash

# set -e
set -x

BUILD_DIR="${BUILD_FOLDER}"
SOURCE_DIR="${GITHUB_WORKSPACE}"

if [[ "${MUDLET_VERSION_BUILD}" == -ptb* ]]; then
  public_test_build="true"
fi

# we deploy only if told to deploy or we run a cron+clang+cmake job (for PTB)
if { [ "${DEPLOY}" = "deploy" ]; } \
#   || { [ "${TRAVIS_EVENT_TYPE}" = "cron" ] &&  [ "${CC}" = "clang" ] && [ "${Q_OR_C_MAKE}" = "cmake" ]; }
then

# coverity is currently defunct as it was dependent on our travis infrastructure. See #4887
#  if [ "$TRAVIS_EVENT_TYPE" = "cron" ] && [ "${DEPLOY}" = "deploy" ]; then
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
#    exit
#  fi

  # We refer to $BUILD_COMMIT in the environment to get the commit data now
  COMMIT_DATE=$(git show -s --format="%cs" | tr -d '-')
  YESTERDAY_DATE=$(date -d "yesterday" '+%F' | tr -d '-')

  git clone https://github.com/Mudlet/installers.git -b add-sentry "${BUILD_DIR}/../installers"

  cd "${BUILD_DIR}/../installers/generic-linux"

  ln -s "${BUILD_DIR}" source

  # unset LD_LIBRARY_PATH as it upsets linuxdeployqt
  export LD_LIBRARY_PATH=

  if ! [[ "$GITHUB_REF" =~ ^"refs/tags/" ]] && [ "${public_test_build}" != "true" ]; then
    echo "== Creating a snapshot build =="
    ./make-installer.sh "${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}"
    cd "${BUILD_DIR}/../installers/generic-linux"

    chmod +x "Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}.AppImage"
    tar -cvf "Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-linux-x64.AppImage.tar" "Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}.AppImage"


    echo "=== ... later, via Github ==="
    # Move the finished file into a folder of its own, because we ask Github to upload contents of a folder
    mkdir "upload/"
    mv "Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-linux-x64.AppImage.tar" "upload/"
    {
      echo "FOLDER_TO_UPLOAD=$(pwd)/upload"
      echo "UPLOAD_FILENAME=Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-linux-x64"
    } >> "$GITHUB_ENV"
    DEPLOY_URL="Github artifact, see https://github.com/$GITHUB_REPOSITORY/runs/$GITHUB_RUN_ID"

  else # ptb/release build
    if [ "${public_test_build}" == "true" ]; then

      if [[ "${COMMIT_DATE}" -lt "${YESTERDAY_DATE}" ]]; then
        echo "== No new commits, aborting public test build generation =="
        exit 0
      fi

      echo "== Creating a public test build =="
    else
      echo "== Creating a release build =="
    fi

    if [ "${public_test_build}" == "true" ]; then
      ./make-installer.sh -pr "${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}"
    else
      ./make-installer.sh -r "${VERSION}"
    fi

    if [ "${public_test_build}" == "true" ]; then
      chmod +x "Mudlet PTB.AppImage"
    else
      chmod +x "Mudlet.AppImage"
    fi

    if [ "${public_test_build}" == "true" ]; then
      tar -cvf "Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-linux-x64.AppImage.tar" "Mudlet PTB.AppImage"
    else
      tar -cvf "Mudlet-${VERSION}-linux-x64.AppImage.tar" "Mudlet.AppImage"
      echo "=== Creating portable version for Linux ==="
      PORTABLE_NAME="Mudlet-${VERSION}-linux-x64-portable"
      touch "portable.txt"
      echo "Created portable.txt file"
      tar -czf "${PORTABLE_NAME}.tar.gz" "Mudlet.AppImage" "portable.txt"
      rm -f "portable.txt"
    fi

    if [ "${public_test_build}" == "true" ]; then
      echo "=== Setting up for Github upload ==="
      mkdir "upload/"
      mv "Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}-linux-x64.AppImage.tar" "upload/"
      {
        echo "FOLDER_TO_UPLOAD=$(pwd)/upload"
        echo "UPLOAD_FILENAME=Mudlet-$VERSION$MUDLET_VERSION_BUILD-${BUILD_COMMIT}-linux-x64"
      } >> "$GITHUB_ENV"
      DEPLOY_URL="Github artifact, see https://github.com/$GITHUB_REPOSITORY/runs/$GITHUB_RUN_ID"
    else
      echo "=== Uploading installer to https://www.mudlet.org/wp-content/files/?C=M;O=D ==="
      scp -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "Mudlet-${VERSION}-linux-x64.AppImage.tar" "mudmachine@mudlet.org:${DEPLOY_PATH}"

      DEPLOY_URL="https://www.mudlet.org/wp-content/files/Mudlet-${VERSION}-linux-x64.AppImage.tar"
      if ! curl --output /dev/null --silent --head --fail "$DEPLOY_URL"; then
        echo "Error: release not found as expected at $DEPLOY_URL"
        exit 1
      fi

      # upload an unzipped, unversioned release for appimage.github.io
      scp -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "Mudlet.AppImage" "mudmachine@mudlet.org:${DEPLOY_PATH}"
      DEPLOY_URL="https://www.mudlet.org/wp-content/files/Mudlet-${VERSION}-linux-x64.AppImage.tar"

      SHA256SUM=$(shasum -a 256 "Mudlet-${VERSION}-linux-x64.AppImage.tar" | awk '{print $1}')
      current_timestamp=$(date "+%-d %-m %Y %-H %-M %-S")
      read -r day month year hour minute second <<< "$current_timestamp"

      curl --retry 5 -X POST 'https://www.mudlet.org/download-add.php' \
      -H "x-wp-download-token: $X_WP_DOWNLOAD_TOKEN" \
      -F "file_type=2" \
      -F "file_remote=$DEPLOY_URL" \
      -F "file_name=Mudlet ${VERSION} (Linux)" \
      -F "file_des=sha256: $SHA256SUM" \
      -F "file_cat=5" \
      -F "file_permission=-1" \
      -F "file_timestamp_day=$day" \
      -F "file_timestamp_month=$month" \
      -F "file_timestamp_year=$year" \
      -F "file_timestamp_hour=$hour" \
      -F "file_timestamp_minute=$minute" \
      -F "file_timestamp_second=$second" \
      -F "output=json" \
      -F "do=Add File"

      echo "=== Uploading portable version ==="
      PORTABLE_NAME="Mudlet-${VERSION}-linux-x64-portable"
      scp -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "${PORTABLE_NAME}.tar.gz" "mudmachine@mudlet.org:${DEPLOY_PATH}"
      PORTABLE_DEPLOY_URL="https://www.mudlet.org/wp-content/files/${PORTABLE_NAME}.tar.gz"

      if ! curl --output /dev/null --silent --head --fail "$PORTABLE_DEPLOY_URL"; then
        echo "Error: portable release not found as expected at $PORTABLE_DEPLOY_URL"
        exit 1
      fi

      PORTABLE_SHA256SUM=$(shasum -a 256 "${PORTABLE_NAME}.tar.gz" | awk '{print $1}')

      curl --retry 5 -X POST 'https://www.mudlet.org/download-add.php' \
      -H "x-wp-download-token: $X_WP_DOWNLOAD_TOKEN" \
      -F "file_type=2" \
      -F "file_remote=$PORTABLE_DEPLOY_URL" \
      -F "file_name=Mudlet ${VERSION} (Linux Portable)" \
      -F "file_des=sha256: $PORTABLE_SHA256SUM" \
      -F "file_cat=5" \
      -F "file_permission=-1" \
      -F "file_timestamp_day=$day" \
      -F "file_timestamp_month=$month" \
      -F "file_timestamp_year=$year" \
      -F "file_timestamp_hour=$hour" \
      -F "file_timestamp_minute=$minute" \
      -F "file_timestamp_second=$second" \
      -F "output=json" \
      -F "do=Add File"
    fi

    # push release to DBLSQD
    sudo npm install -g dblsqd-cli
    dblsqd login -e "https://api.dblsqd.com/v1/jsonrpc" -u "${DBLSQD_USER}" -p "${DBLSQD_PASS}"

    if [ "${public_test_build}" == "true" ]; then
      echo "=== Downloading release feed ==="
      downloadedfeed=$(mktemp)
      wget "https://feeds.dblsqd.com/MKMMR7HNSP65PquQQbiDIw/public-test-build/linux/x86_64" --output-document="$downloadedfeed"
      echo "=== Generating a changelog ==="
      cd "${SOURCE_DIR}" || exit
      changelog=$(lua "${SOURCE_DIR}/CI/generate-changelog.lua" --mode ptb --releasefile "${downloadedfeed}")

      echo "=== Creating release in Dblsqd ==="
      dblsqd release -a mudlet -c public-test-build -m "${changelog}" "${VERSION}${MUDLET_VERSION_BUILD}-${BUILD_COMMIT}" || true

      # release registration and uploading will be manual for the time being
    else
      echo "=== Registering release with Dblsqd ==="
      dblsqd push -a mudlet -c release -r "${VERSION}" -s mudlet --type "standalone" --attach linux:x86_64 "${DEPLOY_URL}"
    fi

    if [ "${public_test_build}" != "true" ]; then
      # generate and deploy source tarball
      cd "${HOME}" || exit
      # get the archive script
      wget https://raw.githubusercontent.com/meitar/git-archive-all.sh/master/git-archive-all.sh

      cd "${SOURCE_DIR}" || exit
      # generate and upload the tarball
      chmod +x "${HOME}/git-archive-all.sh"
      "${HOME}/git-archive-all.sh" "Mudlet-${VERSION}.tar"
      xz "Mudlet-${VERSION}.tar"
      scp -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null "Mudlet-${VERSION}.tar.xz" "mudmachine@mudlet.org:${DEPLOY_PATH}"
      FILE_URL="https://www.mudlet.org/wp-content/files/Mudlet-${VERSION}.tar.xz"
      SHA256SUM=$(shasum -a 256 "Mudlet-${VERSION}.tar.xz" | awk '{print $1}')
      current_timestamp=$(date "+%-d %-m %Y %-H %-M %-S")
      read -r day month year hour minute second <<< "$current_timestamp"

      curl --retry 5 -X POST 'https://www.mudlet.org/download-add.php' \
      -H "x-wp-download-token: $X_WP_DOWNLOAD_TOKEN" \
      -F "file_type=2" \
      -F "file_remote=$FILE_URL" \
      -F "file_name=Mudlet ${VERSION} (Source Code)" \
      -F "file_des=sha256: $SHA256SUM" \
      -F "file_cat=6" \
      -F "file_permission=-1" \
      -F "file_timestamp_day=$day" \
      -F "file_timestamp_month=$month" \
      -F "file_timestamp_year=$year" \
      -F "file_timestamp_hour=$hour" \
      -F "file_timestamp_minute=$minute" \
      -F "file_timestamp_second=$second" \
      -F "output=json" \
      -F "do=Add File"
    fi
  fi
  export DEPLOY_URL
fi
