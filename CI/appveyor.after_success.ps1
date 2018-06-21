if ("$Env:APPVEYOR_REPO_NAME" -ne "Mudlet/Mudlet") {
  exit 0
}

cd "$Env:APPVEYOR_BUILD_FOLDER\src\release"
windeployqt.exe --release mudlet.exe
. "$Env:APPVEYOR_BUILD_FOLDER\CI\copy-non-qt-win-dependencies.ps1"

Remove-Item * -include *.cpp, *.o

if ("$Env:APPVEYOR_REPO_TAG" -eq "false") {
  $DEPLOY_URL = "https://ci.appveyor.com/api/buildjobs/$Env:APPVEYOR_JOB_ID/artifacts/src%2Fmudlet.zip"
} else {
  Write-Output "=== Cloning installer project ==="
  git clone https://github.com/Mudlet/installers.git C:\projects\installers
  cd C:\projects\installers\windows
  
  Write-Output "=== Installing Squirrel for Windows ==="
  nuget install squirrel.windows -ExcludeVersion

  Write-Output "=== Setting up directories ==="
  # credit to http://markwal.github.io/programming/2015/07/28/squirrel-for-windows.html
  $SQUIRRELWIN = "C:\projects\squirrel-packaging-prep"
  $SQUIRRELWINBIN = "$SQUIRRELWIN\lib\net45\"

  if (-not $(Test-Path "$SQUIRRELWINBIN")) {
      New-Item "$SQUIRRELWINBIN" -ItemType "directory"
  }

  Write-Output "=== Moving things to where Squirel expects them ==="
  # move everything into src\release\squirrel.windows\lib\net45\ as that's where Squirrel would like to see it
  Move-Item $Env:APPVEYOR_BUILD_FOLDER\src\release\* $SQUIRRELWINBIN

  Write-Output "=== Creating Nuget package ==="
  nuget pack C:\projects\installers\windows\mudlet.nuspec -Version $($Env:VERSION) -BasePath $SQUIRRELWIN -OutputDirectory $SQUIRRELWIN
  Write-Output "=== Creating installers from Nuget package ==="
  .\squirrel.windows\tools\Squirrel --releasify C:\projects\squirrel-packaging-prep\Mudlet.$($Env:VERSION).nupkg --releaseDir C:\projects\squirreloutput --loadingGif C:\projects\installers\windows\splash-installing-2x.png --no-msi --setupIcon C:\projects\installers\windows\mudlet_main_48px.ico -n "/a /f C:\projects\installers\windows\code-signing-certificate.p12 /p $Env:signing_password /fd sha256 /tr http://timestamp.digicert.com /td sha256"
  Write-Output "=== Removing old directory content of release folder ==="
  Remove-Item -Recurse -Force $Env:APPVEYOR_BUILD_FOLDER\src\release\*
  Write-Output "=== Copying installer over for appveyor ==="
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
