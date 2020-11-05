#!/bin/sh

echo "Running appveyor.after_success.sh shell script..."
echo ""

if [ "${APPVEYOR_REPO_NAME}" != "Mudlet/Mudlet" ]; then
    # Only run this code on the main Mudlet Github repository - do nothing otherwise:
    echo "This does not appear to be running on the main Mudlet repository, packaging is not appropriate....!"
    echo ""
    exit 0
fi

# Source/setup some variables (including PATH):
. /c/projects/mudlet/CI/appveyor.set-build-info.sh

if [ "${ABORT_PT_BUILDS}" = "true" ]; then
    # If the build has been forcible terminated with an "appveyor exit" command
    # the on_success and on_finish steps in yaml file will still run and this
    # script is part of the on_success handling, so quitely exit without doing
    # anything else if the condition that will invoke the exit (this test) is
    # true:
    exit 0
fi

if [ "${BUILD_TYPE}" = "pull_request" ] || [ "${BUILD_TYPE}" = "development" ]; then
    echo "=== Creating a (\"${BUILD_TYPE}\") snapshot build ==="
    echo ""
    echo "Moving to package directory: $(/usr/bin/cygpath --windows "/c/projects/mudlet/package") ..."
    cd /c/projects/mudlet/package || (appveyor AddMessage "ERROR: /c/projects/mudlet/package directory not found! Build aborted." -Category Error ; exit 1)

    mv ./mudlet.exe ./Mudlet.exe
    # Pending support for controllable debug or relWithDebInfo builds:
    # Separate out the debug information
    # ${MINGW_INTERNAL_BASE_DIR}/usr/bin/objcopy.exe --only-keep-debug Mudlet.exe Mudlet.exe.debug
    # ${MINGW_INTERNAL_BASE_DIR}/usr/bin/objcopy.exe --strip-debug Mudlet.exe
    # ${MINGW_INTERNAL_BASE_DIR}/usr/bin/objcopy.exe --add-gnu-debuglink=Mudlet.exe.debug Mudlet.exe

    # Compress everything up (at maximum compression) into an zip archive:
    /usr/bin/zip -rv9 "${ZIP_FILE_NAME}" ./*

    # wget returns the URL that is used, which we need to capture to report it:
    echo "=== Uploading the (\"${BUILD_TYPE}\") snapshot build ==="
    DEPLOY_URL=$(wget --method PUT --body-file="${ZIP_FILE_NAME}" "https://make.mudlet.org/snapshots/${ZIP_FILE_NAME}" -O - -q)
    if [ -n "${DEPLOY_URL}" ]; then
        if [ -n "${APPVEYOR_PULL_REQUEST_NUMBER}" ]; then
            PR_ID=", #${APPVEYOR_PULL_REQUEST_NUMBER}"
        fi
        # This sends a notification message to Gitter - back quotes are used by
        # it to show the enclosed text in a special format!
        wget --post-data "message=Deployed Mudlet \`${VERSION}${MUDLET_VERSION_BUILD}\` (${BUILD_BITNESS}-bit windows${PR_ID}) to [${DEPLOY_URL}](${DEPLOY_URL})" \
            https://webhooks.gitter.im/e/cc99072d43b642c4673a
        echo ""
        appveyor AddMessage "INFORMATION: Deployed the output to ${DEPLOY_URL}" -Category Information
        echo "=== Deployed the output to ${DEPLOY_URL} ==="
        echo ""
        echo "******************************************************"
    fi
    # Also retain the archive as a build artifact (for longer than the 14 days)
    # that we use for our own website - i.e. 6 months, use the FileName option
    # because otherwise the path is unnecessarily kept in the name:
    appveyor PushArtifact "$(/usr/bin/cygpath --windows "/c/projects/mudlet/package/${ZIP_FILE_NAME}")" -FileName "${ZIP_FILE_NAME}"

else # BUILD_TYPE is "public_test" OR "release"
    if [ "${BUILD_TYPE}" = "public_test" ]; then
        echo "=== Creating a public test beta build ==="

        # As Squirrel takes the Start menu name from the binary we need to
        # rename it - however we do not need to mark whether it is 32 or 64 bits
        # here - the choice of having a space in the application name seems
        # unwise but it is what was done in the previous (PowerShell) CI scripts:
        mv /c/projects/mudlet/package/mudlet.exe "/c/projects/mudlet/package/mudlet PTB.exe"

        # Pending support for controllable debug or relWithDebInfo builds:
        # Separate out the debug information
        # ${MINGW_INTERNAL_BASE_DIR}/usr/bin/objcopy.exe --only-keep-debug "Mudlet PTB.exe" "Mudlet PTB.exe.debug"
        # ${MINGW_INTERNAL_BASE_DIR}/usr/bin/objcopy.exe --strip-debug "Mudlet PTB.exe"
        # ${MINGW_INTERNAL_BASE_DIR}/usr/bin/objcopy.exe --add-gnu-debuglink="Mudlet PTB.exe.debug" "Mudlet PTB.exe"
    else
        echo "=== Creating a release build ==="
        # Pending support for controllable debug or relWithDebInfo builds:
        # Separate out the debug information
        # ${MINGW_INTERNAL_BASE_DIR}/usr/bin/objcopy.exe --only-keep-debug Mudlet.exe Mudlet.exe.debug
        # ${MINGW_INTERNAL_BASE_DIR}/usr/bin/objcopy.exe --strip-debug Mudlet.exe
        # ${MINGW_INTERNAL_BASE_DIR}/usr/bin/objcopy.exe --add-gnu-debuglink=Mudlet.exe.debug Mudlet.exe
    fi
    echo ""

    # Get our installer, put it into a directory alongside our existing stuff:
    echo "  Cloning mudlet installer project into $(/usr/bin/cygpath --windows "/c/projects/installers")"
    git clone https://github.com/Mudlet/installers.git /c/projects/installers
    echo ""

    echo "  Moving to installer's Windows sub-directory: $(/usr/bin/cygpath --windows "/c/projects/installers/windows") ..."
    cd /c/projects/installers/windows || (appveyor AddMessage "ERROR: /c/projects/installers/windows directory not found! Build aborted." -Category Error ; exit 1)
    echo ""

    # Install squirrel for Windows here, NuGet 5.1.0 is present as part of the
    # Visual Studio 2019 build image we are using:
    echo "  Installing Squirrel for Windows with NuGet"
echo "TEMP: determining where nuget is:"
which nuget.exe
    # The ExcludeVersion option prevents each NuGet package having a version
    # suffix - which would otherwise make it harder to identify the
    # sub-directory we want
    nuget.exe install squirrel.windows -ExcludeVersion
    echo ""

    echo "  Setting up directories"
    # credit to http://markwal.github.io/programming/2015/07/28/squirrel-for-windows.html
    mkdir -p /c/projects/packaging/lib/Net45
    # also:
    mkdir -p /c/projects/package
    echo ""

    echo "  Moving entire set of files (including subdirectories) wanted in installer to where Squirrel expects them"
    mv /c/projects/mudlet/package/* /c/projects/packaging/lib/Net45
    echo ""

    NUSPEC_FILE="/c/projects/installers/windows/mudlet.nuspec"
    if [ "${BUILD_TYPE}" = "public_test" ]; then
        # allow public test builds to be installed side by side with the release
        # builds by renaming the app in the nuspec file. No dots must be present
        # in the <id> field, see:
        # https://github.com/Squirrel/Squirrel.Windows/blob/master/docs/using/naming.md

        cp ${NUSPEC_FILE} ${NUSPEC_FILE}.orig
        # Changes for PT Builds:
        # 1. "id" Must not have spaces, or END in a number, or some other
        # characters, including any form of brace/bracket. It, and whatever
        # version number/string is used becomes the name of the nuspec and then
        # nupkg output files
        # 2. "title" Used in Windows properties
        # 3. "iconUrl" an online link to the icon used for the installed product
        # in the Windows application list and for the "Setup.exe" - for the
        # former case it is only accessed when the product is installed and not
        # when the product installer is constructed. It is now (Nuget 5.3)
        # depricated in favour of a local "icon" file name, but the version in
        # the AppVeyor CI platform (5.1) cannot use the latter element
# Note that the standard mudlet Windows icon set will change
# from: "mudlet_main_512x512_6XS_icon.ico" to: "mudlet.ico" once
# PR 4089 is merged in:

        echo "  Modifying mudlet.nuspec file for ${BUILD_BITNESS} Bit PTB usage"
        if [ "${BUILD_BITNESS}" = "64" ]; then
            /usr/bin/sed "s|<id>Mudlet</id>|<id>Mudlet_64_-PublicTestBuild</id>|g" ${NUSPEC_FILE}.orig \
                | /usr/bin/sed "s|<title>Mudlet</title>|<title>Mudlet x64 (Public Test Build)</title>|g" \
                | /usr/bin/sed "s|<iconUrl>https://raw.githubusercontent.com/Mudlet/Mudlet/development/src/icons/mudlet.ico</title>|<title>https://raw.githubusercontent.com/Mudlet/Mudlet/development/src/icons/mudlet_ptb.ico</iconUrl>|g" \
                | /usr/bin/sed "s|<iconUrl>https://raw.githubusercontent.com/Mudlet/Mudlet/development/src/icons/mudlet_main_512x512_6XS_icon.ico</title>|<title>https://raw.githubusercontent.com/Mudlet/Mudlet/development/src/icons/mudlet_ptb.ico</iconUrl>|g" > ${NUSPEC_FILE}
        else
            /usr/bin/sed "s|<id>Mudlet</id>|<id>Mudlet-PublicTestBuild</id>|g" ${NUSPEC_FILE}.orig \
                | /usr/bin/sed "s|<title>Mudlet</title>|<title>Mudlet x32 (Public Test Build)</title>|g" \
                | /usr/bin/sed "s|<iconUrl>https://raw.githubusercontent.com/Mudlet/Mudlet/development/src/icons/mudlet.ico</title>|<title>https://raw.githubusercontent.com/Mudlet/Mudlet/development/src/icons/mudlet_ptb.ico</iconUrl>|g" \
                | /usr/bin/sed "s|<iconUrl>https://raw.githubusercontent.com/Mudlet/Mudlet/development/src/icons/mudlet_main_512x512_6XS_icon.ico</title>|<title>https://raw.githubusercontent.com/Mudlet/Mudlet/development/src/icons/mudlet_ptb.ico</iconUrl>|g" > ${NUSPEC_FILE}
        fi

    else # Release build:
        if [ "${BUILD_BITNESS}" = "64" ]; then
            echo "  Modifying mudlet.nuspec file for 64 Bit release usage"
            /usr/bin/sed "s|<id>Mudlet</id>|<id>Mudlet_64_</id>|g" ${NUSPEC_FILE}.orig \
                | /usr/bin/sed "s|<title>Mudlet</title>|<title>Mudlet x64</title>|g" \
                | /usr/bin/sed "s|<iconUrl>https://raw.githubusercontent.com/Mudlet/Mudlet/development/src/icons/mudlet_main_512x512_6XS_icon.ico</title>|<title>https://raw.githubusercontent.com/Mudlet/Mudlet/development/src/icons/mudlet.ico</iconUrl>|g" > ${NUSPEC_FILE}
        else
            /usr/bin/sed "s|<iconUrl>https://raw.githubusercontent.com/Mudlet/Mudlet/development/src/icons/mudlet_main_512x512_6XS_icon.ico</title>|<title>https://raw.githubusercontent.com/Mudlet/Mudlet/development/src/icons/mudlet.ico</iconUrl>|g" ${NUSPEC_FILE}.orig > ${NUSPEC_FILE}
        fi
    fi
    echo ""

/usr/bin/diff -w "${NUSPEC_FILE}.orig" "${NUSPEC_FILE}"
# And temporarily retain it for post mortems:
# This file does not currently get renamed within CI environment but if it is
# downloaded for analysis we need it to be named according to what it refers to:
appveyor PushArtifact "$(/usr/bin/cygpath --windows "${NUSPEC_FILE}")" -FileName "${EXPORT_NUSPEC_FILE}"

    echo "  Using nuget pack to create the package based on the nuspec file:"
    # As well as the files from the Mudlet build there will be 4 additional
    # files, two each: NuGet.Squirrel and Squirrel with types .dll & .pdb
    # i.e. library and debug symbols from the mudlet-installer.
    if [ -n "${SUFFIX_FOR_NUGET}" ]; then
        # A '-' will automagically get inserted between the ${VERSION} and the
        # ${SUFFIX_FOR_NUGET} values:
        nuget pack "$(/usr/bin/cygpath --windows "${NUSPEC_FILE}")" -NonInteractive -NoPackageAnalysis -Version "${VERSION}" -Suffix "${SUFFIX_FOR_NUGET}" -BasePath "/c/projects/packaging" -OutputDirectory "/c/projects/package"
    else
        nuget pack "$(/usr/bin/cygpath --windows "${NUSPEC_FILE}")" -NonInteractive -NoPackageAnalysis -Version "${VERSION}"                               -BasePath "/c/projects/packaging" -OutputDirectory "/c/projects/package"
    fi
    echo ""

    # After running the above a new ".nupkg" file will be in the
    # -OutputDirectory directory - which we will test for:
    echo "  Testing for NuGet package"
    if [ ! -f "/c/projects/package/${NUPKG_FILE}" ]; then
        appveyor AddMessage "ERROR: $(cygpath --windows "/c/projects/package/${NUPKG_FILE}") file not found! Build aborted." -Category Error
        exit 1
    fi
    echo ""
# The file that should be fed into squirrel.windows:
appveyor PushArtifact "$(/usr/bin/cygpath --windows "/c/projects/package/${NUPKG_FILE}")" -FileName "${NUPKG_FILE}"


    echo "  Using squirrel.windows to generate installer"
    # During debug testing (with a PR) secure variables are NOT available - so
    # we cannot sign anything whilst developing this script...
    if [ -n "${signing_password}" ] ;  then
        ./squirrel.windows/tools/Squirrel \
            --releasify="$(/usr/bin/cygpath --windows "/c/projects/package/${NUPKG_FILE}")" \
            --releaseDir="/c/projects/squirel_output" \
            --no-msi \
            --loadingGif="$(/usr/bin/cygpath --windows "${LOADING_GIF_PATHFILE}")" \
            --setupIcon="$(/usr/bin/cygpath --windows "${SETUP_ICON_PATHFILE}")" \
            -n "/a /f C:\projects\installers\windows\code-signing-certificate.p12 /p ${signing_password} /fd sha256 /tr http://timestamp.digicert.com /td sha256"
    else
        echo "Signing password not available, if this is a PR related build this is expected but it means that the installer cannot be signed."
        appveyor AddMessage "Signing password not available, if this is a PR related build this is expected but it means that the installer cannot be signed." -Category Warning
        ./squirrel.windows/tools/Squirrel \
            --releasify="$(/usr/bin/cygpath --windows "/c/projects/package/${NUPKG_FILE}")" \
            --releaseDir="/c/projects/squirel_output" \
            --no-msi \
            --loadingGif="$(/usr/bin/cygpath --windows "${LOADING_GIF_PATHFILE}")" \
            --setupIcon="$(/usr/bin/cygpath --windows "${SETUP_ICON_PATHFILE}")"
    fi
    # We will want to rename the "/c/projects/squirel_output/Setup.exe" file
    # so that it has a unique name when uploaded to our distribution systems
    if [ ! -f /c/projects/squirel_output/Setup.exe ]; then
        appveyor AddMessage "ERROR: Squirrel failed to generate the installer! Build aborted." -Category Error
        echo "Squirrel failed to generate the installer! Build aborted. Log file follows:"
        cat ./squirrel.windows/tools/SquirrelSetup.log
        exit 1
    fi

    if [ "${BUILD_TYPE}" = "public_test" ]; then
        # wget returns the URL that is used, which we need to capture to report it:
        echo "=== Uploading the public test build ==="
        # Needed for rename of Setup.exe to final installer file name:
        if [ -z "${DEPLOY_KEY_PASS}" ]; then
            # NORMAL TECHNIQUE (NOT SECURE) FOR UPLOADING PTB TO MUDLET SERVER
            DEPLOY_URL=$(wget --method PUT --body-file="/c/projects/squirel_output/Setup.exe" "https://make.mudlet.org/snapshots/Mudlet-${VERSION}-ptb-${PTB_DATE}-${COMMIT}-windows-${BUILD_BITNESS}.exe" -O - -q)
            appveyor AddMessage "INFORMATION: Attempted to upload ${DEPLOY_URL} insecurely via wget to mudlet.org" -Category Information

        else
            # TEMPORARY method to develop scp alternative for use with release
            # but can only be done in nonPR builds as it requires a secure
            # variable that is not available then - will be tested by a cronjob
            # run on development AFTER the PR that adds this is merged.
            # Afterward a further PR must switch to using this all the time
            # or revert it BEFORE THE NEXT RELEASE IS RELEASED.
            echo "=== Registering Mudlet SSH keys for ptb upload ==="
            eval "$(ssh-agent -s)"
            # Add the (password protected) private key in CI/mudlet-deploy-key.pem
            if [ "${DISPLAY}" = "" ]; then
                # Non-interactive, fake things so ssh-ask uses an sshaskpass tool
                DISPLAY=1 SSH_ASKPASS_REQUIRE="force" SSH_ASKPASS=/c/projects/mudlet/CI/auto-add-ssh-key.sh /usr/bin/ssh-add /c/projects/mudlet/CI/mudlet-deploy-key.pem < /dev/null
            else
                # Interactive, but force ssh-ask to use an sshaskpass substitute
                SSH_ASKPASS_REQUIRE="force" SSH_ASKPASS=/c/projects/mudlet/CI/auto-add-ssh-key.sh /usr/bin/ssh-add /c/projects/mudlet/CI/mudlet-deploy-key.pem < /dev/null
            fi
            appveyor AddMessage "INFORMATION: Attempted to upload ${DEPLOY_URL} securely via scp to mudlet.org" -Category Information
        fi

        DEPLOY_URL="Mudlet-${VERSION}-ptb-${PTB_DATE}-${COMMIT}-windows-${BUILD_BITNESS}.exe"
        /usr/bin/scp -o StrictHostKeyChecking=no "Mudlet-${VERSION}.tar.xz" "keneanung@mudlet.org:https://make.mudlet.org/snapshots/${DEPLOY_URL}"

        # This sends the notification message to Gitter!
        wget --post-data "message=Deployed ${DEPLOY_URL} (${BUILD_BITNESS}-bit windows PTB for $(date +"%Y/%m/%d")) to [${DEPLOY_URL}](${DEPLOY_URL})" \
            https://webhooks.gitter.im/e/cc99072d43b642c4673a
        echo ""
        appveyor AddMessage "INFORMATION: Deployed the output to ${DEPLOY_URL}" -Category Information
        echo "=== Deployed the output to ${DEPLOY_URL} ==="
        echo ""
        echo "******************************************************"
    else
        # A release build
        echo "=== Registering Mudlet SSH keys for release upload ==="
        eval "$(ssh-agent -s)"
        # Add the (password protected) private key in CI/mudlet-deploy-key.pem
        if [ "${DISPLAY}" = "" ]; then
            # Non-interactive, fake things so ssh-ask uses an sshaskpass tool
            DISPLAY=1 SSH_ASKPASS_REQUIRE="force" SSH_ASKPASS=/c/projects/mudlet/CI/auto-add-ssh-key.sh /usr/bin/ssh-add /c/projects/mudlet/CI/mudlet-deploy-key.pem < /dev/null
        else
            # Interactive, but force ssh-ask to use an sshaskpass substitute
            SSH_ASKPASS_REQUIRE="force" SSH_ASKPASS=/c/projects/mudlet/CI/auto-add-ssh-key.sh /usr/bin/ssh-add /c/projects/mudlet/CI/mudlet-deploy-key.pem < /dev/null
        fi

        echo "=== Uploading installer to https://www.mudlet.org/wp-content/files/?C=M;O=D ==="
        DEPLOY_URL="https://www.mudlet.org/wp-content/files/Mudlet-${VERSION}-windows-x${BUILD_BITNESS}-installer.exe"
        /usr/bin/scp -o StrictHostKeyChecking=no "/c/projects/squirel_output/Setup.exe" "keneanung@mudlet.org:${DEPLOY_PATH}"
        appveyor AddMessage "INFORMATION: uploaded ${DEPLOY_URL} securely via scp to mudlet.org" -Category Information
    fi

# temporarily retain built files for post-mortems
# Log from squirrel.windows
appveyor PushArtifact "$(/usr/bin/cygpath --windows "/c/projects/installers/windows/squirrel.windows/tools/Squirrel-Releasify.log")" -FileName "Squirrel-Releasify.log"

# The "-full.nupkg" file that comes out of squirrel.windows that is supposed to
# be used when an update from a prior version cannot be used - we now suppress
# the generation of the other (much smaller) "-delta.nupkg" file that would be
# the combined differences from the previous version:
# /c/projects/squirrel_output/Mudlet_x64_-PublicTestBuild-4.9.2-ptb20201020-full.nupkg
appveyor PushArtifact "$(/usr/bin/cygpath --windows "/c/projects/squirrel_output/${SQUIRREL_FULL_NUPKG_FILE}")" -FileName "${SQUIRREL_FULL_NUPKG_FILE}"

    # During debug testing (with a PR) secure variables are NOT available - so
    # we cannot access DBLSQD whilst developing this script...
    if [ -n "${DBLSQD_USER}" ];  then
        # Process to push release to DBLSQD
        npm install -g dblsqd-cli

        echo "===Logging in to DBLSQD server==="
        dblsqd login -e "https://api.dblsqd.com/v1/jsonrpc" -u "${DBLSQD_USER}" -p "${DBLSQD_PASS}"

        if [ "${BUILD_TYPE}" = "public_test" ]; then
            echo "=== Downloading release feed ==="
            DOWNLOADED_FEED=$(mktemp)
            if [ "${BUILD_BITNESS}" = "64" ]; then
                wget "https://feeds.dblsqd.com/MKMMR7HNSP65PquQQbiDIw/public-test-build/win/x86_64" --output-document="${DOWNLOADED_FEED}"
            else
                wget "https://feeds.dblsqd.com/MKMMR7HNSP65PquQQbiDIw/public-test-build/win/x86" --output-document="${DOWNLOADED_FEED}"
            fi
            echo ""

            echo "=== Generating a changelog for PTB ==="
            # This MUST be run from the "./CI" subdirectory of the source code to
            # access Mudlet's own Lua modules:
            cd /c/projects/mudlet/CI || (appveyor AddMessage "ERROR: /c/projects/mudlet/CI directory not found! Build aborted." -Category Error ; exit 1)
            CHANGLELOG=$("${MINGW_INTERNAL_BASE_DIR}/bin/lua5.1.exe" "/c/projects/mudlet/CI/generate-ptb-changelog.lua" --releasefile "${DOWNLOADED_FEED}")
            cd /c/projects/installers/windows || (appveyor AddMessage "ERROR: /c/projects/installers/windows directory not found! Build aborted." -Category Error ; exit 1)
            echo "Changelog:
${CHANGLELOG}\
--------------------------------------------------------------------------------"
            appveyor AddMessage "CHANGLELOG contents:\
${CHANGLELOG}" -Category Information

            echo "=== Creating PTB in Dblsqd ==="
            dblsqd release -a mudlet -c public-test-build -m "${CHANGLELOG}" "${VERSION}${MUDLET_VERSION_BUILD}" || true

            echo "=== Registering PTB with Dblsqd ==="
            if [ "${BUILD_BITNESS}" = "64" ]; then
                dblsqd push -a mudlet -c public-test-build -r "${VERSION}${MUDLET_VERSION_BUILD}" -s mudlet --type "standalone" --attach win:x86_64 "${DEPLOY_URL}"
            else
                dblsqd push -a mudlet -c public-test-build -r "${VERSION}${MUDLET_VERSION_BUILD}" -s mudlet --type "standalone" --attach win:x86 "${DEPLOY_URL}"
            fi
        else
            echo "=== Registering release with Dblsqd ==="
            if [ "${BUILD_BITNESS}" = "64" ]; then
                dblsqd push -a mudlet -c release -r "${VERSION}" -s mudlet --type "standalone" --attach win:x86_64 "${DEPLOY_URL}"
            else
                dblsqd push -a mudlet -c release -r "${VERSION}" -s mudlet --type "standalone" --attach win:x86 "${DEPLOY_URL}"
            fi
        fi

        # This sends a notification message to Gitter - back quotes are used by
        # it to show the enclosed text in a special format!
        wget --post-data "message=Deployed Mudlet \`${VERSION}${MUDLET_VERSION_BUILD}\` (${BUILD_BITNESS}-bit windows${PR_ID}) to [${DEPLOY_URL}](${DEPLOY_URL})" \
            https://webhooks.gitter.im/e/cc99072d43b642c4673a
        echo ""
        appveyor AddMessage "INFORMATION: Deployed the output to ${DEPLOY_URL}" -Category Information
        echo "=== Deployed the output to ${DEPLOY_URL} ==="
        echo ""
        echo "******************************************************"
        fi
    fi

echo ""
echo "=== Finished building a ${BUILD_BITNESS} bit Mudlet ${VERSION}${MUDLET_VERSION_BUILD} ==="
echo ""
echo "   ... appveyor.after_success.sh shell script finished!"
echo ""
echo "******************************************************"
