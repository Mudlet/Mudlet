if ("$Env:APPVEYOR_REPO_NAME" -ne "Mudlet/Mudlet") {
  exit 0
}

cd "$Env:APPVEYOR_BUILD_FOLDER\src\release"
windeployqt.exe --release mudlet.exe
. "$Env:APPVEYOR_BUILD_FOLDER\CI\copy-non-qt-win-dependencies.ps1"

Remove-Item * -include *.cpp, *.o

$Script:PublicTestBuild = if ($Env:MUDLET_VERSION_BUILD) { $Env:MUDLET_VERSION_BUILD.StartsWith('-ptb') } else { $FALSE }

if (Test-Path Env:APPVEYOR_PULL_REQUEST_NUMBER) {
  $Script:Commit = git rev-parse --short $Env:APPVEYOR_PULL_REQUEST_HEAD_COMMIT
} else {
  $Script:Commit = git rev-parse --short HEAD
}
# ensure sha part always starts with a character due to https://github.com/Squirrel/Squirrel.Windows/issues/1394
$Script:VersionAndSha = "$Env:VERSION-ptb$Script:Commit"

if ("$Env:APPVEYOR_REPO_TAG" -eq "false" -and -Not $Script:PublicTestBuild) {
  Write-Output "=== Creating a snapshot build ==="
  Rename-Item -Path "$Env:APPVEYOR_BUILD_FOLDER\src\release\mudlet.exe" -NewName "Mudlet.exe"
  cmd /c 7z a Mudlet-%VERSION%%MUDLET_VERSION_BUILD%-windows.zip "%APPVEYOR_BUILD_FOLDER%\src\release\*"

  Set-Variable -Name "uri" -Value "https://make.mudlet.org/snapshots/Mudlet-$env:VERSION$env:MUDLET_VERSION_BUILD-windows.zip";
  Set-Variable -Name "inFile" -Value "Mudlet-$env:VERSION$env:MUDLET_VERSION_BUILD-windows.zip";
  Set-Variable -Name "outFile" -Value "upload-location.txt";
  Invoke-RestMethod -Uri $uri -Method PUT -InFile $inFile -OutFile $outFile;

  $DEPLOY_URL = Get-Content -Path $outFile -Raw
} else {
  if ($Script:PublicTestBuild) {
    Write-Output "=== Creating a public test build ==="
    # Squirrel takes Start menu name from the binary
  Rename-Item -Path "$Env:APPVEYOR_BUILD_FOLDER\src\release\mudlet.exe" -NewName "Mudlet PTB.exe"
  } else {
    Write-Output "=== Creating a release build ==="
    Rename-Item -Path "$Env:APPVEYOR_BUILD_FOLDER\src\release\mudlet.exe" -NewName "Mudlet.exe"
  }

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

  $Script:NuSpec = "C:\projects\installers\windows\mudlet.nuspec"
  Write-Output "=== Creating Nuget package ==="
  if ($Script:PublicTestBuild) {
    # allow public test builds to be installed side by side with the release builds by renaming the app
    # no dots in the <id>: https://github.com/Squirrel/Squirrel.Windows/blob/master/docs/using/naming.md
    (Get-Content "$Script:NuSpec").replace('<id>Mudlet</id>', '<id>Mudlet-PublicTestBuild</id>') | Set-Content "$Script:NuSpec"
    (Get-Content "$Script:NuSpec").replace('<title>Mudlet</title>', '<title>Mudlet (Public Test Build)</title>') | Set-Content "$Script:NuSpec"
  }
  nuget pack "$Script:NuSpec" -Version "$Script:VersionAndSha" -BasePath $SQUIRRELWIN -OutputDirectory $SQUIRRELWIN

  Write-Output "=== Creating installers from Nuget package ==="
  if ($Script:PublicTestBuild) {
    $TestBuildString = "-PublicTestBuild"
  } else {
    $TestBuildString = ""
  }

  $nupkg_path = "C:\projects\squirrel-packaging-prep\Mudlet$TestBuildString.$Script:VersionAndSha.nupkg"
  if (-not (Test-Path -Path $nupkg_path -PathType Leaf)) {
    Write-Output "=== ERROR: nupkg doesn't exist as expected! Build aborted."
    exit 1
  }

  # fails silently if the nupkg file is not found
  .\squirrel.windows\tools\Squirrel --releasify $nupkg_path --releaseDir C:\projects\squirreloutput --loadingGif C:\projects\installers\windows\splash-installing-2x.png --no-msi --setupIcon C:\projects\installers\windows\mudlet_main_48px.ico -n "/a /f C:\projects\installers\windows\code-signing-certificate.p12 /p $Env:signing_password /fd sha256 /tr http://timestamp.digicert.com /td sha256"

  Write-Output "=== Removing old directory content of release folder ==="
  Remove-Item -Recurse -Force $Env:APPVEYOR_BUILD_FOLDER\src\release\*
  Write-Output "=== Copying installer over for appveyor ==="
  Move-Item C:\projects\squirreloutput\* $Env:APPVEYOR_BUILD_FOLDER\src\release

  if (-not (Test-Path -Path "${Env:APPVEYOR_BUILD_FOLDER}\src\release\Setup.exe" -PathType Leaf)) {
    Write-Output "=== ERROR: Squirrel failed to generate the installer! Build aborted. Squirrel log is:"
    Get-Content -Path .\squirrel.windows\tools\SquirrelSetup.log
    exit 1
  }

  if ($Script:PublicTestBuild) {
    Write-Output "=== Uploading public test build to make.mudlet.org ==="
    Set-Variable -Name "uri" -Value "https://make.mudlet.org/snapshots/Mudlet-$env:VERSION$env:MUDLET_VERSION_BUILD-windows.exe";
    Set-Variable -Name "inFile" -Value "${Env:APPVEYOR_BUILD_FOLDER}\src\release\Setup.exe";
    Set-Variable -Name "outFile" -Value "upload-location.txt";
    Invoke-RestMethod -Uri $uri -Method PUT -InFile $inFile -OutFile $outFile;

    $DEPLOY_URL = Get-Content -Path $outFile -Raw
    $DEPLOY_URL = $DEPLOY_URL.Trim()
    echo "Deployed Mudlet to '$DEPLOY_URL'"
  } else {
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
    Write-Output "=== Uploading installer to https://www.mudlet.org/wp-content/files/?C=M;O=D ==="
    $session.Open($sessionOptions)
    $session.PutFiles("${Env:APPVEYOR_BUILD_FOLDER}\src\release\Setup.exe", "${Env:DEPLOY_PATH}/Mudlet-${Env:VERSION}-windows-installer.exe")
    $session.Close()
    $session.Dispose()
    $DEPLOY_URL="https://www.mudlet.org/wp-content/files/Mudlet-${Env:VERSION}-windows-installer.exe"
  }

  Write-Output "=== Installing dblsqd-cli ==="
  npm install -g dblsqd-cli
  dblsqd login -e "https://api.dblsqd.com/v1/jsonrpc" -u "${Env:DBLSQD_USER}" -p "${Env:DBLSQD_PASS}"

  if ($Script:PublicTestBuild) {
    Write-Output "=== Creating release in Dblsqd ==="
    dblsqd release -a mudlet -c public-test-build -m "(changelogs for public test builds are not yet available)" "${Env:VERSION}${Env:MUDLET_VERSION_BUILD}".ToLower()

    Write-Output "=== Registering release with Dblsqd ==="
    dblsqd push -a mudlet -c public-test-build -r "${Env:VERSION}${Env:MUDLET_VERSION_BUILD}".ToLower() -s mudlet --type "standalone" --attach win:x86 "${DEPLOY_URL}"
  } else {
    Write-Output "=== Registering release with Dblsqd ==="
    dblsqd push -a mudlet -c release -r "${Env:VERSION}" -s mudlet --type "standalone" --attach win:x86 "${DEPLOY_URL}"
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
