#!/bin/sh

echo "Running appveyor.after_success.sh shell script..."

if [ ${APPVEYOR_REPO_NAME} != "Mudlet/Mudlet" ] ; then
    # Only run this code on the main Mudlet Github repository - do nothing otherwise:
    echo "This does not appear to be running on the main Mudlet repository, packaging is not appropriate....!"
    echo ""
    exit 0
fi

# Source/setup some variables (including PATH):
. $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER}/CI/appveyor.set-build-info.sh)

if [ ${APPVEYOR_REPO_TAG} = "false" ] && [ ${PUBLIC_TEST_BUILD} != "true" ] ; then
    echo "=== Creating a snapshot build ==="
    ZIP_FILE_NAME=Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-win${BUILD_BITNESS}.zip
    mv $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER}/package/mudlet.exe) $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER}/package/Mudlet.exe)

    /usr/bin/zip -rv9 ${ZIP_FILE_NAME} ./*

    export DEPLOY_URL=$(wget --method PUT --body-file=${ZIP_FILE_NAME} \
        "https://make.mudlet.org/snapshots/${ZIP_FILE_NAME}" -O - -q)

else
    # ensure sha part always starts with a character due to
    # https://github.com/Squirrel/Squirrel.Windows/issues/1394 :
    export VERSION_AND_SHA=${VERSION}-ptb${COMMIT}

    # TODO - create sh script equivalent of part of the powershell script
    # appveyor.after_success.ps1 that produces the squirrel update package
    # the following is a dummy command to keep the shell happy:
    true

fi

echo "   ... appveyor.after_success.sh shell script finished!"
echo ""
echo "******************************************************"
echo ""
echo "Finished building a ${BUILD_BITNESS} bit Mudlet ${VERSION}${MUDLET_VERSION_BUILD}"
if [ -n "${DEPLOY_URL}" ]; then
    if [ -n "${APPVEYOR_PULL_REQUEST_NUMBER}" ] ; then
        PRID=", #${APPVEYOR_PULL_REQUEST_NUMBER}"
    else
        PRID=""
    fi
    wget --post-data "message=Deployed Mudlet ``${VERSION}${MUDLET_VERSION_BUILD}`` (windows ${BUILD_BITNESS}-bit${prId}) to [appveyor]($DEPLOY_URL)" https://webhooks.gitter.im/e/cc99072d43b642c4673a
    echo ""
    echo "Deployed the output to ${DEPLOY_URL}"
fi
echo ""
echo "******************************************************"
