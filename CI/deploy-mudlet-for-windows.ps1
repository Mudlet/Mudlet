. "$Env:GITHUB_WORKSPACE\CI\set-build-info-for-windows.ps1"

if ($Env:GITHUB_REPO_NAME -ne "Mudlet/Mudlet") {
  exit 0
}

cd "$Env:GITHUB_WORKSPACE\package-MINGW32-release"

#$Script:QtVersionRegex = [regex]'\\([\d\.]+)\\mingw'
#$Script:QtVersion = $QtVersionRegex.Match($Env:QT_BASE_DIR).Groups[1].Value
#if ([version]$Script:QtVersion -ge [version]'5.14.0') {
#  windeployqt.exe mudlet.exe
#}
#else {
#  windeployqt.exe --release mudlet.exe
#}

#. "$Env:APPVEYOR_BUILD_FOLDER\CI\copy-non-qt-win-dependencies.ps1"

Remove-Item * -include *.cpp, *.o

$Script:PublicTestBuild = if ($Env:MUDLET_VERSION_BUILD) { $Env:MUDLET_VERSION_BUILD.StartsWith('-ptb') } else { $FALSE }

if ($Env:GITHUB_REPO_TAG -eq "false" -and -Not $Script:PublicTestBuild) {
  Write-Output "=== Creating a snapshot build ==="
  Rename-Item -Path "$Env:GITHUB_WORKSPACE\package-MINGW32-release\mudlet.exe" -NewName "Mudlet.exe"
  cmd /c 7z a Mudlet-%VERSION%%MUDLET_VERSION_BUILD%-%BUILD_COMMIT%-windows.zip "%GITHUB_WORKSPACE%\package-MINGW32-release\*"

  #Set-Variable -Name "uri" -Value "https://make.mudlet.org/snapshots/Mudlet-$env:VERSION$env:MUDLET_VERSION_BUILD-$env:BUILD_COMMIT-windows.zip";
  #Set-Variable -Name "inFile" -Value "Mudlet-$env:VERSION$env:MUDLET_VERSION_BUILD-$env:BUILD_COMMIT-windows.zip";
  $uploadFilename = "Mudlet-$env:VERSION$env:MUDLET_VERSION_BUILD-$env:BUILD_COMMIT-windows.zip"
  #Set-Variable -Name "outFile" -Value "upload-location.txt";
  #Write-Output "=== Uploading the snapshot build ==="
  #Invoke-RestMethod -Uri $uri -Method PUT -InFile $inFile -OutFile $outFile;

  #$DEPLOY_URL = Get-Content -Path $outFile -Raw
  Write-Output "=== Setting up upload directory ==="
  $uploadDir = "$Env:GITHUB_WORKSPACE\upload"

  if (-not $(Test-Path "$uploadDir")) {
    New-Item "$uploadDir" -ItemType "directory"
  }

  Write-Output "=== Moving files to upload directory ==="
  Move-Item $Env:GITHUB_WORKSPACE\package-MINGW32-release\$uploadFilename $uploadDir
  
  echo "FOLDER_TO_UPLOAD=$uploadDir" | Out-File -Append -FilePath $Env:GITHUB_ENV
  echo "UPLOAD_FILENAME=$uploadFilename" | Out-File -Append -FilePath $Env:GITHUB_ENV
} else {
  if ($Script:PublicTestBuild) {

    $COMMIT_DATE = Get-Date -date $(git show -s --format="%cs")
    $YESTERDAY_DATE = $(Get-Date).AddDays(-1).Date
    if ($COMMIT_DATE -lt $YESTERDAY_DATE) {
      Write-Output "=== No new commits, aborting public test build generation ==="
      exit 0
    }

    Write-Output "=== Creating a public test build ==="
    # Squirrel takes Start menu name from the binary
    Rename-Item -Path "$Env:GITHUB_WORKSPACE\package-MINGW32-release\mudlet.exe" -NewName "Mudlet PTB.exe"
    # ensure sha part always starts with a character due to https://github.com/Squirrel/Squirrel.Windows/issues/1394
    $Script:VersionAndSha = "$Env:VERSION-ptb-$Env:BUILD_COMMIT"
  } else {
    Write-Output "=== Creating a release build ==="
    Rename-Item -Path "$Env:GITHUB_WORKSPACE\package-MINGW32-release\mudlet.exe" -NewName "Mudlet.exe"
    $Script:VersionAndSha = "$Env:VERSION"
  }

  Write-Output "=== Cloning installer project ==="
  git clone https://github.com/Mudlet/installers.git $Env:GITHUB_WORKSPACE\installers
  cd $Env:GITHUB_WORKSPACE\installers\windows

  Write-Output "=== Installing Squirrel for Windows ==="
  nuget install squirrel.windows -ExcludeVersion

  Write-Output "=== Setting up directories ==="
  # credit to http://markwal.github.io/programming/2015/07/28/squirrel-for-windows.html
  $SQUIRRELWIN = "$Env:GITHUB_WORKSPACE\squirrel-packaging-prep"
  $SQUIRRELWINBIN = "$SQUIRRELWIN\lib\net45\"

  if (-not $(Test-Path "$SQUIRRELWINBIN")) {
    New-Item "$SQUIRRELWINBIN" -ItemType "directory"
  }

  Write-Output "=== Moving things to where Squirrel expects them ==="
  # move everything into package-MINGW32-release\squirrel.windows\lib\net45\ as that's where Squirrel would like to see it
  Move-Item $Env:GITHUB_WORKSPACE\package-MINGW32-release\* $SQUIRRELWINBIN

  $Script:NuSpec = "${Env:GITHUB_WORKSPACE}\installers\windows\mudlet.nuspec"
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
    $InstallerIconFile = "${Env:GITHUB_WORKSPACE}\src\icons\mudlet_ptb.ico"
  } else {
    $TestBuildString = ""
    $InstallerIconFile = "${Env:GITHUB_WORKSPACE}\src\icons\mudlet.ico"
  }

  $nupkg_path = "${Env:GITHUB_WORKSPACE}\squirrel-packaging-prep\Mudlet$TestBuildString.$Script:VersionAndSha.nupkg"
  if (-not (Test-Path -Path $nupkg_path -PathType Leaf)) {
    Write-Output "=== ERROR: nupkg doesn't exist as expected! Build aborted."
    exit 1
  }

  # fails silently if the nupkg file is not found
  .\squirrel.windows\tools\Squirrel --releasify $nupkg_path --releaseDir ${Env:GITHUB_WORKSPACE}\squirreloutput --loadingGif ${Env:GITHUB_WORKSPACE}\installers\windows\splash-installing-2x.png --no-msi --setupIcon $InstallerIconFile -n "/a /f ${Env:GITHUB_WORKSPACE}\installers\windows\code-signing-certificate.p12 /p $Env:signing_password /fd sha256 /tr http://timestamp.digicert.com /td sha256"
  Write-Output "=== Removing old directory content of release folder ==="
  Remove-Item -Recurse -Force $Env:GITHUB_WORKSPACE\package-MINGW32-release\*
  Write-Output "=== Copying installer over ==="
  Move-Item $Env:GITHUB_WORKSPACE\squirreloutput\* $Env:GITHUB_WORKSPACE\package-MINGW32-release

  if (-not (Test-Path -Path "${Env:GITHUB_WORKSPACE}\package-MINGW32-release\Setup.exe" -PathType Leaf)) {
    Write-Output "=== ERROR: Squirrel failed to generate the installer! Build aborted. Squirrel log is:"
    if (Test-Path -Path ".\squirrel.windows\tools\SquirrelSetup.log" -PathType Leaf) {
      echo "SquirrelSetup.log: "
      Get-Content -Path .\squirrel.windows\tools\SquirrelSetup.log
    }
    if (Test-Path -Path ".\squirrel.windows\tools\Squirrel-Releasify.log" -PathType Leaf) {
      echo "Squirrel-Releasify.log: "
      Get-Content -Path .\squirrel.windows\tools\Squirrel-Releasify.log
    }
    exit 1
  }

  if ($Script:PublicTestBuild) {
    Write-Output "=== Uploading public test build to make.mudlet.org ==="
    Set-Variable -Name "uri" -Value "https://make.mudlet.org/snapshots/Mudlet-$env:VERSION$env:MUDLET_VERSION_BUILD-$env:BUILD_COMMIT-windows.exe";
    Set-Variable -Name "inFile" -Value "${GITHUB_WORKSPACE}\package-MINGW32-release\Setup.exe";
    Set-Variable -Name "outFile" -Value "upload-location.txt";
    Invoke-RestMethod -Uri $uri -Method PUT -InFile $inFile -OutFile $outFile;

    $DEPLOY_URL = Get-Content -Path $outFile -Raw
    $DEPLOY_URL = $DEPLOY_URL.Trim()
    echo "Deployed Mudlet to '$DEPLOY_URL'"
  } #else {
    # get winscp .NET dll for uploads
    # activate higher TLS version. Seems PS only uses 1.0 by default
    # credit: https://stackoverflow.com/questions/41618766/powershell-invoke-webrequest-fails-with-ssl-tls-secure-channel/48030563#48030563
    
    
    #[Net.ServicePointManager]::SecurityProtocol = [System.Security.Authentication.SslProtocols] "tls, tls11, tls12"
    #(New-Object System.Net.WebClient).DownloadFile("https://downloads.sourceforge.net/project/winscp/WinSCP/5.13.4/WinSCP-5.13.4-Automation.zip?r=https%3A%2F%2Fsourceforge.net%2Fprojects%2Fwinscp%2Ffiles%2FWinSCP%2F5.13.4%2FWinSCP-5.13.4-Automation.zip%2Fdownload&ts=1538514946", "C:\src\Winscp-automation.zip")
    #Add-Type -AssemblyName System.IO.Compression.FileSystem
    #[System.IO.Compression.ZipFile]::ExtractToDirectory("$Env:GITHUB_WORKSPACE\Winscp-automation.zip", "$Env:GITHUB_WORKSPACE\Winscp-automation\")
    #Add-Type -Path '$Env:GITHUB_WORKSPACE\Winscp-automation\WinSCPnet.dll'

    # do the upload
    #$sessionOptions = New-Object WinSCP.SessionOptions -Property @{
    #  # sftp://
    #  Protocol = [WinSCP.Protocol]::Scp
    #  HostName = "mudlet.org"
    #  UserName = "vadi"
    #  SshPrivateKeyPath = "$Env:GITHUB_WORKSPACE\CI\mudlet-deploy-key-windows.ppk"
    #  SshPrivateKeyPassphrase = "${Env:DEPLOY_KEY_PASS}"
    #}
    #$session = New-Object WinSCP.Session
    #$fingerprint =  $session.ScanFingerprint($sessionOptions, "SHA-256")
    #$sessionOptions.SshHostKeyFingerprint = $fingerprint
    ## Connect
    #Write-Output "=== Uploading installer to https://www.mudlet.org/wp-content/files/?C=M;O=D ==="
    #$session.Open($sessionOptions)
    #$session.PutFiles("${Env:GITHUB_WORKSPACE}\src\release\Setup.exe", "${Env:DEPLOY_PATH}/Mudlet-${Env:VERSION}-windows-installer.exe")
    #$session.Close()
    #$session.Dispose()
    #$DEPLOY_URL="https://www.mudlet.org/wp-content/files/Mudlet-${Env:VERSION}-windows-installer.exe"
    #Set-Variable -Name "SHA256SUM" -Value (Get-FileHash -Path "${Env:DEPLOY_PATH}/Mudlet-${Env:VERSION}-windows-installer.exe" -Algorithm SHA256).Hash
    #Invoke-RestMethod -Uri 'https://www.mudlet.org/wp-content/plugins/wp-downloadmanager/download-add.php' `
    #-Method POST `
    #-Headers @{
    #    "x-wp-download-token" = $Env:X_WP_DOWNLOAD_TOKEN
    #} `
    #-Body @{
    #    "file_type" = "2"
    #    "file_remote" = $Env:DEPLOY_URL
    #    "file_name" = "Mudlet-$Env:VERSION (Windows)"
    #    "file_des" = "sha256: $SHA256SUM"
    #    "file_cat" = "0"
    #    "file_permission" = "-1"
    #    "output" = "json"
    #    "do" = "Add File"
    #}
    #Push-AppveyorArtifact "${Env:GITHUB_WORKSPACE}\src\release\Setup.exe" -DeploymentName "${Env:DEPLOY_PATH}/Mudlet-${Env:VERSION}-windows-installer.exe"
  #}

  Write-Output "=== Installing dblsqd-cli ==="
  npm install -g dblsqd-cli
  dblsqd login -e "https://api.dblsqd.com/v1/jsonrpc" -u "${Env:DBLSQD_USER}" -p "${Env:DBLSQD_PASS}"

  if ($Script:PublicTestBuild) {
    Write-Output "=== Downloading release feed ==="
    $Script:DownloadedFeed = [System.IO.Path]::GetTempFileName()
    Invoke-WebRequest "https://feeds.dblsqd.com/MKMMR7HNSP65PquQQbiDIw/public-test-build/win/x86" -OutFile $Script:DownloadedFeed
    Write-Output "=== Generating a changelog ==="
    Push-Location "$Env:GITHUB_WORKSPACE\CI\"
    $Script:Changelog = lua "${Env:GITHUB_WORKSPACE}\CI\generate-changelog.lua" --mode ptb --releasefile $Script:DownloadedFeed
    Pop-Location
    Write-Output $Script:Changelog
    Write-Output "=== Creating release in Dblsqd ==="
    
    #Remote Write-Output this when verified correct
    Write-Output "dblsqd release -a mudlet -c public-test-build -m $Script:Changelog "${Env:VERSION}${Env:MUDLET_VERSION_BUILD}-${Env:BUILD_COMMIT}".ToLower()"

    Write-Output "=== Registering release with Dblsqd ==="
    #Remote Write-Output this when verified correct
    Write-Output "dblsqd push -a mudlet -c public-test-build -r '${Env:VERSION}${Env:MUDLET_VERSION_BUILD}-${Env:BUILD_COMMIT}".ToLower() -s mudlet --type "standalone" --attach win:x86 "${DEPLOY_URL}'"
  }
  #  else {
  #   Write-Output "=== Registering release with Dblsqd ==="
  #   dblsqd push -a mudlet -c release -r "${Env:VERSION}" -s mudlet --type "standalone" --attach win:x86 "${DEPLOY_URL}"
  # }
}

if ($Env:GITHUB_PULL_REQUEST_NUMBER) {
  $prId = " ,#${Env:GITHUB_PULL_REQUEST_NUMBER}"
}

echo ""
echo "******************************************************"
echo ""
if ("$Env:MUDLET_VERSION_BUILD" -eq "") {
  # A release build
  echo "Finished building Mudlet $Env:VERSION"
} else {
  # Not a release build so include the Git SHA1 in the message
  echo "Finished building Mudlet $Env:VERSION$Env:MUDLET_VERSION_BUILD-$Env:BUILD_COMMIT"
}
if (Test-Path variable:DEPLOY_URL) {
    echo "Deployed the output to $DEPLOY_URL"
}
echo ""
echo "******************************************************"
