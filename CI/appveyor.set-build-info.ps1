cd $Env:APPVEYOR_BUILD_FOLDER

if ($Env:APPVEYOR_REPO_TAG -eq "false") {
  # The only scheduled Appveyor builds are public test builds
  if ($Env:APPVEYOR_SCHEDULED_BUILD -eq "True") {
    $Env:MUDLET_VERSION_BUILD = "-ptb"
  } else {
    $Env:MUDLET_VERSION_BUILD = "-testing"
  }
  if (Test-Path Env:APPVEYOR_PULL_REQUEST_NUMBER) {
      # APPVEYOR_PULL_REQUEST_HEAD_COMMIT is the development branch commit that
      # a Pull-Request is merged onto - as such we do not want this for
      # labelling a PR - instead we want APPVEYOR_REPO_COMMIT
      $Env:BUILD_COMMIT = git rev-parse --short $Env:APPVEYOR_REPO_COMMIT
      $Env:MUDLET_VERSION_BUILD = "$Env:MUDLET_VERSION_BUILD-PR$Env:APPVEYOR_PULL_REQUEST_NUMBER"
  } else {
    $Env:BUILD_COMMIT = git rev-parse --short HEAD

    if ($Env:MUDLET_VERSION_BUILD -eq "-ptb") {
      $Script:Date = Get-Date -Format "yyyy-MM-dd"
      $Env:MUDLET_VERSION_BUILD = "$Env:MUDLET_VERSION_BUILD-$Date"
    }
  }
}

# not all systems we deal with allow uppercase ascii characters
$Env:MUDLET_VERSION_BUILD = "$Env:MUDLET_VERSION_BUILD".ToLower()
$Env:BUILD_COMMIT = "$Env:BUILD_COMMIT".ToLower()

$VersionLine = Select-String -Pattern "Version =" $Env:APPVEYOR_BUILD_FOLDER/src/mudlet.pro
$VersionRegex = [regex]'= {1}(.+)$'
$Env:VERSION = $VersionRegex.Match($VersionLine).Groups[1].Value

if ($Env:MUDLET_VERSION_BUILD -eq "") {
  # A release build maybe?
  Write-Output "BUILDING MUDLET $Env:VERSION$Env:MUDLET_VERSION_BUILD"
} else {
  # Otherwise we should report the Git SHA1 which is no longer in MUDLET_VERSION_BUILD:
  Write-Output "BUILDING MUDLET $Env:VERSION$Env:MUDLET_VERSION_BUILD-$Env:BUILD_COMMIT"
}
