#!/bin/bash

set -e

# we deploy only qmake and clang combination for macOS
if [ "${Q_OR_C_MAKE}" = "qmake" ] && [ "${CC}" = "gcc" ]; then
  git clone https://github.com/Mudlet/installers.git "${TRAVIS_BUILD_DIR}/../installers"

  cd "${TRAVIS_BUILD_DIR}/../installers/generic-linux"

  ln -s "${TRAVIS_BUILD_DIR}" source

  if [ -z "${TRAVIS_TAG}" ]; then
    bash make-installer.sh "${VERSION}${MUDLET_VERSION_BUILD}"

    DEPLOY_URL=$(wget --method PUT --body-file="Mudlet-${VERSION}${MUDLET_VERSION_BUILD}.AppImage"  "https://transfer.sh/Mudlet-${VERSION}${MUDLET_VERSION_BUILD}.AppImage" -O - -q)
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

