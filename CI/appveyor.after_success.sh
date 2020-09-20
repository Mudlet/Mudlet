#!/bin/sh

echo "Running appveyor.after_success.sh shell script..."
echo ""

if [ ${APPVEYOR_REPO_NAME} != "Mudlet/Mudlet" ] ; then
    # Only run this code on the main Mudlet Github repository - do nothing otherwise:
    echo "This does not appear to be running on the main Mudlet repository, packaging is not appropriate....!"
    echo ""
    exit 0
fi

# Source/setup some variables (including PATH):
. $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER}/CI/appveyor.set-build-info.sh)

cd $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER}/package)

if [ ${APPVEYOR_REPO_TAG} = "false" ] && [ ${PUBLIC_TEST_BUILD} != "true" ] ; then
    echo "  Creating a snapshot build"
    echo ""
    # Now append an "-x32" or "-x64" suffix to the "windows" to match the linux
    # snapshot file:
    ZIP_FILE_NAME=Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-windows-x${BUILD_BITNESS}.zip
    mv $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER}/package/mudlet.exe) $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER}/package/Mudlet.exe)

    /usr/bin/zip -rv9 ${ZIP_FILE_NAME} ./*

    export DEPLOY_URL=$(wget --method PUT --body-file=${ZIP_FILE_NAME} \
        "https://make.mudlet.org/snapshots/${ZIP_FILE_NAME}" -O - -q)

else
    # ensure sha part always starts with a character due to
    # https://github.com/Squirrel/Squirrel.Windows/issues/1394 :
    export VERSION_AND_SHA=${VERSION}-ptb${COMMIT}
    if [ "${public_test_build}" == "true" ]; then
# TEMPORARILY BODGE THINGS FOR TESTING (2 of 2):
#        COMMIT_DATE=$(git show -s --format=%as | tr -d '-')
#        YESTERDAYS_DATE=$(date -v-1d '+%F' | tr -d '-')
#
#        if [ "${COMMIT_DATE}" -lt "${YESTERDAYS_DATE}" ] ; then
#            echo ""
#            echo "Finished building a ${BUILD_BITNESS} bit Mudlet ${VERSION}${MUDLET_VERSION_BUILD}"
#            echo ""
#            echo "No new commits, aborting public test build generation"
#            echo ""
#            echo "   ... appveyor.after_success.sh shell script finished!"
#            echo ""
#            echo "******************************************************"
#            exit 0
#        fi
        echo "  Creating a public test beta build"
        # As Squirrel takes Start menu name from the binary we need to rename
        # the binary:
        mv $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER}/package/mudlet.exe) $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER}/package/Mudlet PTB.exe)
    else
        echo "  Creating a release build"
        mv $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER}/package/mudlet.exe) $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER}/package/Mudlet.exe)
    fi
    echo ""

    # Get our installer, put it into a directory alongside our existing stuff:
    echo "  Cloning mudlet installer project"
    git clone https://github.com/Mudlet/installers.git /c/projects/installers
    echo ""

    # Install squirrel for Windows, NuGet 5.1.0 is present as part of the
    # Visual Studio 2019 build image we are using:
    echo "  Installing Squirrel for Windows with NuGet"
    mkdir -p /c/projects/installers/windows
    cd /c/projects/installers/windows
echo "TEMP: determining where nuget is:"
where nuget.exe
which nuget.exe
    nuget.exe install squirrel.windows -ExcludeVersion
    echo ""

    echo "  Setting up directories"
    # credit to http://markwal.github.io/programming/2015/07/28/squirrel-for-windows.html
    SQUIRRELWIN=$(/usr/bin/cygpath --windows "/c/projects/squirrel-packaging-prep")
    SQUIRRELWINBIN=$(/usr/bin/cygpath --windows "/c/projects/squirrel-packaging-prep/lib/net45")
    mkdir -p ${SQUIRRELWINBIN}
    echo ""

    echo "  Moving things to where Squirrel expects them"
    # move everything into src\release\squirrel.windows\lib\net45\ as that's where Squirrel would like to see it
    mv $(/usr/bin/cygpath --unix "${APPVEYOR_BUILD_FOLDER}/package") $(/usr/bin/cygpath --unix "${SQUIRRELWINBIN}")
    echo ""

    NU_SPEC_FILE="/c/projects/installers/windows/mudlet.nuspec"
    if [ "${PUBLIC_TEST_BUILD}" = "true" ]; then
        # allow public test builds to be installed side by side with the release
        # builds by renaming the app in the nuspec file. No dots must be present
        # in the <id> field, see:
        # https://github.com/Squirrel/Squirrel.Windows/blob/master/docs/using/naming.md

        echo "  Modifying mudlet.nuspec file for PTB usage"
        cp ${NU_SPEC_FILE} ${NU_SPEC_FILE}.orig
        /usr/bin/sed "s|<id>Mudlet</id>|<id>Mudlet-PublicTestBuild</id>|g" ${NU_SPEC_FILE}.orig \
            | /usr/bin/sed "s|<title>Mudlet</title>|<title>Mudlet (Public Test Build)</title>|g" > ${NU_SPEC_FILE}
        echo ""

echo "TEMP: ensuring we have modified the nuspec file:"
/usr/bin/diff -w ${NU_SPEC_FILE}.orig ${NU_SPEC_FILE}
echo ""
    }

    echo "  Creating the package based on the nuspec file:"
    nuget pack "$(/usr/bin/cygpath --windows "${NU_SPEC_FILE}")" -Version "${VERSION_AND_SHA}" -BasePath "$(/usr/bin/cygpath --windows "${SQUIRRELWIN}")" -OutputDirectory "$(/usr/bin/cygpath --windows "${SQUIRRELWIN}")"
    echo ""

    echo "  Testing for NuGet package"
    if [ "${PUBLIC_TEST_BUILD}" = "true" ]; then
        PACKAGE_BUILD_FILE="$(/usr/bin/cygpath --windows "/c/projects/squirrel-packaging-prep/Mudlet-PublicTestBuild.${VersionAndSha}.nupkg")"
    else
        PACKAGE_BUILD_FILE="$(/usr/bin/cygpath --windows "/c/projects/squirrel-packaging-prep/Mudlet.${VersionAndSha}.nupkg")"
    fi
    if [ ! -f "$(/usr/bin/cygpath --windows "${PACKAGE_BUILD_FILE}")" ] ; then
        echo "  ERROR: $(/usr/bin/cygpath --unix "${PACKAGE_BUILD_FILE}") file not found! Build aborted"
        exit 1
    fi
fi

echo "  TESTED OKAY SO FAR! - Build termining early, to confirm process working so far"
echo ""
echo "Finished building a ${BUILD_BITNESS} bit Mudlet ${VERSION}${MUDLET_VERSION_BUILD}"
exit 0

if [ -n "${DEPLOY_URL}" ] ; then
    if [ -n "${APPVEYOR_PULL_REQUEST_NUMBER}" ] ; then
        prId=", #${APPVEYOR_PULL_REQUEST_NUMBER}"
    fi
    wget --post-data "message=Deployed Mudlet \`${VERSION}${MUDLET_VERSION_BUILD}\` (${BUILD_BITNESS}-bit windows ${prId}) to [${DEPLOY_URL}](${DEPLOY_URL})" \
        https://webhooks.gitter.im/e/cc99072d43b642c4673a
    echo ""
    echo "Deployed the output to ${DEPLOY_URL}"
    echo ""
    echo "******************************************************"
fi
echo ""
echo "   ... appveyor.after_success.sh shell script finished!"
echo ""
echo "******************************************************"
