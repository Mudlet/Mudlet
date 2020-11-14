$Script:Build_Folder = $Env:APPVEYOR_BUILD_FOLDER ? $Env:APPVEYOR_BUILD_FOLDER : $Env:GITHUB_WORKSPACE

Set-Location $Script:Build_Folder

if ($Env:APPVEYOR_REPO_TAG -ne "true" -and -not $env:GITHUB_REF.StartsWith("refs/tags/")) {
  # The only scheduled builds are public test builds
  if ($Env:APPVEYOR_SCHEDULED_BUILD -eq "True" -or $Env:GITHUB_EVENT_NAME -eq "schedule") {
    $Env:MUDLET_VERSION_BUILD = "-ptb"
  } else {
    $Env:MUDLET_VERSION_BUILD = "-testing"
  }
  if (Test-Path Env:APPVEYOR_PULL_REQUEST_NUMBER) {
    $Script:Commit = git rev-parse --short $Env:APPVEYOR_PULL_REQUEST_HEAD_COMMIT
    $Env:MUDLET_VERSION_BUILD = "$Env:MUDLET_VERSION_BUILD-PR$Env:APPVEYOR_PULL_REQUEST_NUMBER-$Commit"
  }  elseif ($Env:GITHUB_EVENT_NAME -eq "pull_request") {
    $Script:Commit = git rev-parse --short $Env:GITHUB_SHA
    $Script:PR_NUMBER = $Env:GITHUB_REF.Substring("refs/pull/".Length).Trim()
    $Env:MUDLET_VERSION_BUILD = "$Env:MUDLET_VERSION_BUILD-PR$Script:PR_NUMBER-$Commit"
  } else {
    $Script:Commit = git rev-parse --short HEAD

    if ($Env:MUDLET_VERSION_BUILD -eq "-ptb") {
      $Script:Date = Get-Date -Format "yyyy-MM-dd"
      $Script:Short_Commit = $Script:Commit.Substring(0, 5)
      $Env:MUDLET_VERSION_BUILD = "$Env:MUDLET_VERSION_BUILD-$Date-$Short_Commit"
    } else {
      $Env:MUDLET_VERSION_BUILD = "$Env:MUDLET_VERSION_BUILD-$Commit"
    }
  }
}

# not all systems we deal with allow uppercase ascii characters
$Env:MUDLET_VERSION_BUILD = "$Env:MUDLET_VERSION_BUILD".ToLower()

$VersionLine = Select-String -Pattern "Version =" $Script:Build_Folder/src/mudlet.pro
$VersionRegex = [regex]'= {1}(.+)$'
$Env:VERSION = $VersionRegex.Match($VersionLine).Groups[1].Value

Write-Output "BUILDING MUDLET $Env:VERSION$Env:MUDLET_VERSION_BUILD"
