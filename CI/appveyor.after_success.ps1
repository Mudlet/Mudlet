if ("$Env:APPVEYOR_REPO_NAME" -ne "Mudlet/Mudlet") {
  exit 0
}

# we deploy only QT 5.6 for windows at the moment
if ("$Env:QT_BASE_DIR" -eq "C:\Qt\5.6\mingw49_32") {

  cd "$Env:APPVEYOR_BUILD_FOLDER\src\release"
  windeployqt.exe --release mudlet.exe
  COPY $Env:MINGW_BASE_DIR\lib\libyajl.dll .
  COPY C:\src\lua-5.1.5\lua-5.1.5\src\lua51.dll .
  COPY C:\src\openssl-1.0.2l\libeay32.dll .
  COPY C:\src\openssl-1.0.2l\ssleay32.dll .
  COPY $Env:MINGW_BASE_DIR\bin\libzip-5.dll .
  COPY $Env:MINGW_BASE_DIR\bin\libhunspell-1.4-0.dll .
  COPY $Env:MINGW_BASE_DIR\bin\libpcre-1.dll .
  COPY $Env:MINGW_BASE_DIR\bin\libsqlite3-0.dll .
  COPY $Env:MINGW_BASE_DIR\bin\zlib1.dll .
  XCOPY /S /I /Q ..\mudlet-lua mudlet-lua
  COPY ..\*.dic .
  COPY C:\src\luazip\luazip-master\zip.dll .
  XCOPY /S /I /Q $Env:MINGW_BASE_DIR\lib\lua\5.1 .

  Remove-Item * -include *.cpp, *.o

# temporary commenting to get appveyor squirrel packing to work
#  if ("$Env:APPVEYOR_REPO_TAG" -eq "false") {
#      $DEPLOY_URL = "https://ci.appveyor.com/api/buildjobs/$Env:APPVEYOR_JOB_ID/artifacts/src%2Fmudlet.zip"
#  } else {
    # C:\src\installbuilder-qt-installer.exe --mode unattended --unattendedmodeui none
    git clone https://github.com/Mudlet/mudlet-installers.git C:\projects\installers
    cd C:\projects\installers\windows
    nuget install secure-file -ExcludeVersion
    nuget install squirrel.windows -ExcludeVersion

    # credit to http://markwal.github.io/programming/2015/07/28/squirrel-for-windows.html
    $SQUIRRELWIN = "$Env:APPVEYOR_BUILD_FOLDER\src\release\squirrel.windows\"
    $SQUIRRELWINBIN = "$Env:APPVEYOR_BUILD_FOLDER\src\release\squirrel.windows\lib\net45\"

    if (-not $(Test-Path "$SQUIRRELWINBIN")) {
        New-Item "$SQUIRRELWINBIN" -ItemType "directory"
#    }

    nuget pack mudlet.nuspec -Version $($Env:VERSION) -BasePath $(SQUIRRELWIN) -OutputDirectory $(SQUIRRELWIN)
    Squirrel --releasify build/squirrel.windows/GpxUi.$($Env:VERSION).nupkg --releaseDir=$(SQUIRRELWIN)release
 
   <#
    This is the shell version:
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
    fi #>

  }
}

if (Test-Path Env:APPVEYOR_PULL_REQUEST_NUMBER) {
  $prId = " ,#$Env:APPVEYOR_PULL_REQUEST_NUMBER"
}

if (Test-Path variable:DEPLOY_URL) {
  Invoke-WebRequest -Method POST -Body "message=Deployed Mudlet ``$Env:VERSION$Env:MUDLET_VERSION_BUILD`` (windows${prId}) to [appveyor]($DEPLOY_URL)" https://webhooks.gitter.im/e/cc99072d43b642c4673a
}

echo ""
echo "******************************************************"
echo ""
echo "Finished building Mudlet $Env:VERSION$Env:MUDLET_VERSION_BUILD"
if (Test-Path variable:DEPLOY_URL) {
    echo "Deployed the output to $DEPLOY_URL"
}
echo ""
echo "******************************************************"
