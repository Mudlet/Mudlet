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

  # get winscp .NET dll for uploads
  # activate higher TLS version. Seems PS only uses 1.0 by default
  # credit: https://stackoverflow.com/questions/41618766/powershell-invoke-webrequest-fails-with-ssl-tls-secure-channel/48030563#48030563
  [Net.ServicePointManager]::SecurityProtocol = [System.Security.Authentication.SslProtocols] "tls, tls11, tls12"
  (New-Object System.Net.WebClient).DownloadFile("https://downloads.sourceforge.net/project/winscp/WinSCP/5.13.4/WinSCP-5.13.4-Automation.zip?r=https%3A%2F%2Fsourceforge.net%2Fprojects%2Fwinscp%2Ffiles%2FWinSCP%2F5.13.4%2FWinSCP-5.13.4-Automation.zip%2Fdownload&ts=1538514946", "C:\src\Winscp-automation.zip")
  Add-Type -AssemblyName System.IO.Compression.FileSystem
  [System.IO.Compression.ZipFile]::ExtractToDirectory("C:\src\Winscp-automation.zip", "C:\src\Winscp-automation\")
  Add-Type -Path 'C:\src\Winscp-automation\WinSCPnet.dll'

  # do the upload
  $sessionOptions = New-Object WinSCP.SessionOptions -Property @{
    # sftp://
    Protocol = [WinSCP.Protocol]::Scp
    HostName = "mudlet.org"
    UserName = "keneanung"
    SshPrivateKeyPath = "$Env:APPVEYOR_BUILD_FOLDER\CI\mudlet-deploy-key-windows.ppk"
    SshPrivateKeyPassphrase = "${Env:DEPLOY_KEY_PASS}"
  }
  $session = New-Object WinSCP.Session
  $fingerprint =  $session.ScanFingerprint($sessionOptions, "SHA-256")
  $sessionOptions.SshHostKeyFingerprint = $fingerprint
  # Connect
  $session.Open($sessionOptions)
  $session.PutFiles("${Env:APPVEYOR_BUILD_FOLDER}\src\release\Setup.exe", "${Env:DEPLOY_PATH}/Mudlet-${Env:VERSION}-windows-installer.exe")
  $session.Close()
  $session.Dispose()
  $DEPLOY_URL="http://www.mudlet.org/wp-content/files/Mudlet-${Env:VERSION}-windows-installer.exe"
  npm install -g dblsqd-cli
  dblsqd login -e "https://api.dblsqd.com/v1/jsonrpc" -u "${Env:DBLSQD_USER}" -p "${Env:DBLSQD_PASS}"
  dblsqd push -a mudlet -c release -r "${Env:VERSION}" -s mudlet --type "standalone" --attach win:x86 "${DEPLOY_URL}"
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
