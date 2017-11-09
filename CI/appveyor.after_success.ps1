if ("$Env:APPVEYOR_REPO_NAME" -ne "Mudlet/Mudlet") {
  exit 0
}

# we deploy only QT 5.6 for windows at the moment
if ("$Env:QT_BASE_DIR" -eq "C:\Qt\5.6\mingw49_32") {

  cd "$Env:APPVEYOR_BUILD_FOLDER\src\release"
  echo "1 in this path:"
  echo (Get-Item -Path ".\" -Verbose).FullName
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
  echo "2 in this path:"
  echo (Get-Item -Path ".\" -Verbose).FullName

# temporary commenting to get appveyor squirrel packing to work
#  if ("$Env:APPVEYOR_REPO_TAG" -eq "false") {
#      $DEPLOY_URL = "https://ci.appveyor.com/api/buildjobs/$Env:APPVEYOR_JOB_ID/artifacts/src%2Fmudlet.zip"
#  } else {
    # C:\src\installbuilder-qt-installer.exe --mode unattended --unattendedmodeui none
    git clone https://github.com/Mudlet/installers.git -b squirrel-windows C:\projects\installers
    cd C:\projects\installers\windows
  echo "3 in this path:"
  echo (Get-Item -Path ".\" -Verbose).FullName
    nuget install secure-file -ExcludeVersion
    nuget install squirrel.windows -ExcludeVersion
    echo "ran nuget install"

    # credit to http://markwal.github.io/programming/2015/07/28/squirrel-for-windows.html
    $SQUIRRELWIN = "C:\projects\squirrel-packaging-prep"
    $SQUIRRELWINBIN = "$SQUIRRELWIN\lib\net45\"

    if (-not $(Test-Path "$SQUIRRELWINBIN")) {
        New-Item "$SQUIRRELWINBIN" -ItemType "directory"
    }

    # move everything into src\release\squirrel.windows\lib\net45\ as that's where Squirrel would like to see it
    Move-Item $Env:APPVEYOR_BUILD_FOLDER\src\release\* $SQUIRRELWINBIN

    nuget pack C:\projects\installers\windows\mudlet.nuspec -Version $($Env:VERSION) -BasePath $SQUIRRELWIN -OutputDirectory $SQUIRRELWIN
    echo "ran nuget"
    .\squirrel.windows\tools\Squirrel --releasify C:\projects\squirrel-packaging-prep\Mudlet.$($Env:VERSION).nupkg --releaseDir=C:\projects\squirreloutput -g C:\projects\installers\windows\splash-installing.png
    echo "ran squirrel. C:\projects\squirreloutput is:"
    Get-ChildItem -Path C:\projects\squirreloutput
  echo "4 in this path:"
  echo (Get-Item -Path ".\" -Verbose).FullName
  echo "SQUIRRELWIN is:"
  echo $SQUIRRELWIN
  echo "src\release is:"
  echo $Env:APPVEYOR_BUILD_FOLDER\src\release
  Remove-Item -Recurse -Force $Env:APPVEYOR_BUILD_FOLDER\src\release\*
  Move-Item C:\projects\squirreloutput\* $Env:APPVEYOR_BUILD_FOLDER\src\release

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

  # }
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
