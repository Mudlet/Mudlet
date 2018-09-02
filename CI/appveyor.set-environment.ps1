function SetQtBaseDir([string] $logFile) {
  if(!(Test-Path Env:QT_BASE_DIR)){
    try
    {
      $Env:QT_BASE_DIR = Get-Command "qmake.exe" -ErrorAction Stop | Select-Object -ExpandProperty definition | Split-Path -Parent | Split-Path -Parent
    }
    catch
    {
      $Env:QT_BASE_DIR = "C:\Qt\5.11.0\mingw53_32"
    }
  }
  Write-Output "Using $Env:QT_BASE_DIR as QT base directory." | Tee-Object -File "$logFile" -Append
}

function SetMingwBaseDir([string] $logFile) {
  if(!(Test-Path Env:MINGW_BASE_DIR)){
    $tmp = $Env:QT_BASE_DIR.Split("\\")
    $tmp[-2] = "Tools"
    $tmp[-1] = $tmp[-1] -replace "_32", "*"
    $tmp = $tmp -join "\" | Resolve-Path
    if($tmp -is [array]){
      $tmp = $tmp[-1]
    }
    $Env:MINGW_BASE_DIR = $tmp
  }
  Write-Output "Using $Env:MINGW_BASE_DIR as MinGW base directory." | Tee-Object -File "$logFile" -Append

  if(!(Test-Path Env:MINGW_BASE_DIR_BASH)){
    $Env:MINGW_BASE_DIR_BASH = $Env:MINGW_BASE_DIR -replace "\\", "/" -replace "C:", "/c"
  }
}