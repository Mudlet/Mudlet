cd $Env:APPVEYOR_BUILD_FOLDER

if ($Env:APPVEYOR_REPO_TAG -eq "false") {
  # The only scheduled Appveyor builds are public test builds
  if ($Env:APPVEYOR_SCHEDULED_BUILD -eq "True") {
    $Env:MUDLET_VERSION_BUILD = "-ptb"
  } else {
    $Env:MUDLET_VERSION_BUILD = "-testing"
  }
  if (Test-Path Env:APPVEYOR_PULL_REQUEST_NUMBER) {
      # AppVeyor builds of PRs merge the PR head onto the current development
      # branch creating a new commit - as such we need to refer to the
      # commit Git SHA1 supplied to us rather than trying to back track to the
      # ancestor in the "working" tree (though it does mean the code state is
      # not accurately described as it only reports PR's head without reference
      # to the state of the development at the time of the build:
      $Env:BUILD_COMMIT = git rev-parse --short $Env:APPVEYOR_PULL_REQUEST_HEAD_COMMIT
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
$Env:SENTRY_ORG = "mehul-mathur"
$Env:SENTRY_PROJECT = "native-qt-windows"
$Env:SENTRY_AUTH_TOKEN = "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"
$Env:SENTRY_DSN = "https://f6e9db5d18fcf0d1fd241f96e6dff72a@o4506746708361216.ingest.us.sentry.io/4507080912797696"

$VersionLine = Select-String -Pattern "Version =" $Env:APPVEYOR_BUILD_FOLDER/src/mudlet.pro
$VersionRegex = [regex]'= {1}(.+)$'
$Env:VERSION = $VersionRegex.Match($VersionLine).Groups[1].Value

if ($Env:MUDLET_VERSION_BUILD -eq "") {
  # A release build maybe?
  Write-Output "BUILDING MUDLET $Env:VERSION"
} else {
  # Otherwise we should report the Git SHA1 which is no longer in MUDLET_VERSION_BUILD:
  Write-Output "BUILDING MUDLET $Env:VERSION$Env:MUDLET_VERSION_BUILD-$Env:BUILD_COMMIT"
}
