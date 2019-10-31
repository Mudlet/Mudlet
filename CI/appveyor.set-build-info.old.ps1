cd $Env:APPVEYOR_BUILD_FOLDER

if ($Env:APPVEYOR_REPO_TAG -eq "false") {
  $Env:MUDLET_VERSION_BUILD = "-testing"
  if (Test-Path Env:APPVEYOR_PULL_REQUEST_NUMBER) {
      $Script:Commit = git rev-parse --short $Env:APPVEYOR_PULL_REQUEST_HEAD_COMMIT
      $Env:MUDLET_VERSION_BUILD = "$Env:MUDLET_VERSION_BUILD-PR$Env:APPVEYOR_PULL_REQUEST_NUMBER-$Commit"
  } else {
    $Script:Commit = git rev-parse --short HEAD
    $Env:MUDLET_VERSION_BUILD = "$Env:MUDLET_VERSION_BUILD-$Commit"
  }
}

$VersionLine = Select-String -Pattern "Version =" $Env:APPVEYOR_BUILD_FOLDER/src/mudlet.pro
$VersionRegex = [regex]'= {1}(.+)$'
$Env:VERSION = $VersionRegex.Match($VersionLine).Groups[1].Value

Write-Output "BUILDING MUDLET $Env:VERSION$Env:MUDLET_VERSION_BUILD"
