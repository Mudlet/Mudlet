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

if [ "${ABORT_PT_BUILDS}" = "true" ]; then
    # If the build has been forcible terminated with an "appveyor exit" command
    # the on_success and on_finish steps in yaml file will still run and this
    # script is part of the on_success handling, so quitely exit without doing
    # anything else if the condition that will invoke the exit (this test) is
    # true:
    exit 0
fi

cd $(/usr/bin/cygpath --unix ${APPVEYOR_BUILD_FOLDER}/package)

if [ "${BUILD_TYPE}" = "pull_request" ] || [ "${BUILD_TYPE}" = "development" ] ; then
    echo "=== Creating a snapshot build ==="
    echo ""
    # Now append an "-x32" or "-x64" suffix to the "windows" to match the linux
    # snapshot file:
    ZIP_FILE_NAME=Mudlet-${VERSION}${MUDLET_VERSION_BUILD}-windows-x${BUILD_BITNESS}.zip
    mv $(/usr/bin/cygpath --unix "${APPVEYOR_BUILD_FOLDER}/package/mudlet.exe") $(/usr/bin/cygpath --unix "${APPVEYOR_BUILD_FOLDER}/package/Mudlet.exe")
    # Pending support for debug or relWithDebInfo builds:
    # Separate out the debug information
    # pushd $(/usr/bin/cygpath --unix "${APPVEYOR_BUILD_FOLDER}/package")
    # ${MINGW_INTERNAL_BASE_DIR}/usr/bin/objcopy.exe --only-keep-debug Mudlet.exe Mudlet.exe.debug
    # ${MINGW_INTERNAL_BASE_DIR}/usr/bin/objcopy.exe --strip-debug Mudlet.exe
    # ${MINGW_INTERNAL_BASE_DIR}/usr/bin/objcopy.exe --add-gnu-debuglink=Mudlet.exe.debug Mudlet.exe
    # popd

    # Compress everything up (at maximum compression) into an zip archive:
    /usr/bin/zip -rv9 ${ZIP_FILE_NAME} ./*

    # wget returns the URL that is used, which we need to capture to report it:
    echo "=== Uploading the snapshot build ==="
    export DEPLOY_URL=$(wget --method PUT --body-file=${ZIP_FILE_NAME} "https://make.mudlet.org/snapshots/${ZIP_FILE_NAME}" -O - -q)

    # Also retain the archive as a build artifact (for longer than the 14 days)
    # that we use for our own website - i.e. 6 months, use the rename option
    # because otherwise the path is unnecessarily kept in the name:
    appveyor PushArtifact $(/usr/bin/cygpath --windows "${APPVEYOR_BUILD_FOLDER}/package/${ZIP_FILE_NAME}") -FileName ${ZIP_FILE_NAME}

else # BUILD_TYPE is "public_test" OR "release"
    if [ "${BUILD_TYPE}" = "public_test" ]; then
        echo "  Creating a public test beta build"
        # As Squirrel takes Start menu name from the binary we need to rename it:
        pushd $(/usr/bin/cygpath --unix "${APPVEYOR_BUILD_FOLDER}/package")
        mv mudlet.exe "Mudlet PTB.exe"
        # Pending support for debug/relWithDebInfo builds:
        # Separate out the debug information
        # ${MINGW_INTERNAL_BASE_DIR}/usr/bin/objcopy.exe --only-keep-debug "Mudlet PTB.exe" "Mudlet PTB.exe.debug"
        # ${MINGW_INTERNAL_BASE_DIR}/usr/bin/objcopy.exe --strip-debug "Mudlet PTB.exe"
        # ${MINGW_INTERNAL_BASE_DIR}/usr/bin/objcopy.exe --add-gnu-debuglink="Mudlet PTB.exe.debug" "Mudlet PTB.exe"
        # popd
    else
        echo "  Creating a release build"
        # Pending support for debug/relWithDebInfo builds:
        # Separate out the debug information
        # pushd $(/usr/bin/cygpath --unix "${APPVEYOR_BUILD_FOLDER}/package")
        # ${MINGW_INTERNAL_BASE_DIR}/usr/bin/objcopy.exe --only-keep-debug Mudlet.exe Mudlet.exe.debug
        # ${MINGW_INTERNAL_BASE_DIR}/usr/bin/objcopy.exe --strip-debug Mudlet.exe
        # ${MINGW_INTERNAL_BASE_DIR}/usr/bin/objcopy.exe --add-gnu-debuglink=Mudlet.exe.debug Mudlet.exe
        # popd
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
which nuget.exe
    nuget.exe install squirrel.windows -ExcludeVersion
    echo ""

    echo "  Setting up directories"
    # credit to http://markwal.github.io/programming/2015/07/28/squirrel-for-windows.html
    # Possibly important - C:\projects\squirrel-packaging-prop is a new empty
    # directory:
    NUGET_OUTPUT_DIR=$(/usr/bin/cygpath --windows "/c/projects/nuget_output")
    SQUIRREL_DIR=$(/usr/bin/cygpath --windows "/c/projects/squirrel_output")
    # As the nuget/squirrel stuff is built on a NET 4.5 framework the stuff
    # we want to include in the project must be in a ./lib/Net45 sud-directory:
    SQUIRREL_OUTPUT_DIR=$(/usr/bin/cygpath --windows "${SQUIRREL_DIR}\\lib\\Net45")
    mkdir -p $(/usr/bin/cygpath --unix "${SQUIRREL_OUTPUT_DIR}")
    echo ""

    echo "  Moving things to where Squirrel expects them"
    mv $(/usr/bin/cygpath --unix "${APPVEYOR_BUILD_FOLDER}/package") $(/usr/bin/cygpath --unix "${SQUIRREL_OUTPUT_DIR}")
    echo ""

    NUSPEC_FILE=/c/projects/installers/windows/mudlet.nuspec
    if [ "${BUILD_TYPE}" = "public_test" ]; then
        # allow public test builds to be installed side by side with the release
        # builds by renaming the app in the nuspec file. No dots must be present
        # in the <id> field, see:
        # https://github.com/Squirrel/Squirrel.Windows/blob/master/docs/using/naming.md

        echo "  Modifying mudlet.nuspec file for PTB usage"
        cp ${NUSPEC_FILE} ${NUSPEC_FILE}.orig
        # Changes for PT Builds:
        # 1. "id" Must not have spaces, or END in a number, or some other
        # characters, including any form of brace/bracket. It, and whatever
        # version number/string is used becomes the name of the nuspec and then
        # nupkg output files
        # 2. "title"
        # 3. "iconUrl" an online link to the icon used for the installed product
        # in the Windows application list and for the "Setup.exe" - for the
        # former case it is only accessed when the product is installed and not
        # when the product installer is constructed. It is now (Nuget 5.3)
        # depricated in favour of a local "icon" file name, but the version in
        # the AppVeyor CI platform (5.1) cannot use the latter element
# Note that the standard mudlet Windows icon set will change
# from: "mudlet_main_512x512_6XS_icon.ico" to: "mudlet.ico" once
# PR 4089 is merged in:
        if [ "${BUILD_BITNESS}" = "64" ] ; then
            /usr/bin/sed "s|<id>Mudlet</id>|<id>Mudlet_64_-PublicTestBuild</id>|g" ${NUSPEC_FILE}.orig \
                | /usr/bin/sed "s|<title>Mudlet</title>|<title>Mudlet x64 (Public Test Build)</title>|g" \
                | /usr/bin/sed "s|<iconUrl>https://raw.githubusercontent.com/Mudlet/Mudlet/development/src/icons/mudlet_main_512x512_6XS_icon.ico</title>|<title>https://raw.githubusercontent.com/Mudlet/Mudlet/development/src/icons/mudlet_ptb.ico</iconUrl>|g" > ${NUSPEC_FILE}
        else
            /usr/bin/sed "s|<id>Mudlet</id>|<id>Mudlet-PublicTestBuild</id>|g" ${NUSPEC_FILE}.orig \
                | /usr/bin/sed "s|<title>Mudlet</title>|<title>Mudlet x32 (Public Test Build)</title>|g" \
                | /usr/bin/sed "s|<iconUrl>https://raw.githubusercontent.com/Mudlet/Mudlet/development/src/icons/mudlet_main_512x512_6XS_icon.ico</title>|<title>https://raw.githubusercontent.com/Mudlet/Mudlet/development/src/icons/mudlet_ptb.ico</iconUrl>|g" > ${NUSPEC_FILE}
        fi
        INSTALLER_ICON_FILE="$(/usr/bin/cygpath --windows "${APPVEYOR_BUILD_FOLDER}/src/icons/mudlet_ptb.ico")"
        echo ""

    else
        echo "  Modifying mudlet.nuspec file for build bitness"
        if [ "${BUILD_BITNESS}" = "64" ] ; then
            /usr/bin/sed "s|<id>Mudlet</id>|<id>Mudlet_64_</id>|g" ${NUSPEC_FILE}.orig \
                | /usr/bin/sed "s|<title>Mudlet</title>|<title>Mudlet x64</title>|g" > ${NUSPEC_FILE}
        else
            /usr/bin/sed "s|<title>Mudlet</title>|<title>Mudlet x64</title>|g" ${NUSPEC_FILE}.orig > ${NUSPEC_FILE}
        fi
        INSTALLER_ICON_FILE="$(/usr/bin/cygpath --windows "${APPVEYOR_BUILD_FOLDER}/src/icons/mudlet_main_512x512_6XS_icon.ico")"
    fi

    echo "TEMP: ensuring we have modified the nuspec file:"
    /usr/bin/diff -w ${NUSPEC_FILE}.orig ${NUSPEC_FILE}
    echo ""

    echo "  Creating the package based on the nuspec file:"
    # As well as the files from the Mudlet build there will be 4 additional
    # files, two each: NuGet.Squirrel and Squirrel with types .dll & .pdb
    # i.e. library and debug symbols from the mudlet-installer.
    #  /c/Tools/NuGet/nuget.exe pack "$(/usr/bin/cygpath --windows "${NUSPEC_FILE}")" -Version "4.9.1.20200922" -BasePath "$(/usr/bin/cygpath --windows "${SQUIRRELWIN}")" -OutputDirectory "$(/usr/bin/cygpath --windows "${SQUIRRELWIN}")"
    nuget pack "$(/usr/bin/cygpath --windows "${NUSPEC_FILE}")" -Version "${VERSION_AND_SHA}" -BasePath "$(/usr/bin/cygpath --windows "${SQUIRRELWIN}")" -OutputDirectory "$(/usr/bin/cygpath --windows "${SQUIRRELWIN}")"
    echo ""
    # After running the above a new ".nupkg" file will be in the ${SQUIRRELWIN}
    # directory - which we will test for:
    echo "  Testing for NuGet package"
    if [ ! -f "$(/usr/bin/cygpath --windows "${NUPKG_FILE}")" ] ; then
        echo "  ERROR: $(/usr/bin/cygpath --unix "${NUPKG_FILE}") file not found! Build aborted"
        exit 1
    fi

    .\squirrel.windows\tools\Squirrel --releasify $nupkg_path --releaseDir C:\projects\squirreloutput --loadingGif C:\projects\installers\windows\splash-installing-2x.png --no-msi --setupIcon C:\projects\installers\windows\mudlet_main_48px.ico -n "/a /f C:\projects\installers\windows\code-signing-certificate.p12 /p $Env:signing_password /fd sha256 /tr http://timestamp.digicert.com /td sha256"
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
