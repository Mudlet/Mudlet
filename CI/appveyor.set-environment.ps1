if(!(Test-Path Env:QT_BASE_DIR)){
  try
  {
    $Env:QT_BASE_DIR = Get-Command "qmake.exe" | Select-Object -ExpandProperty definition | Split-Path -Parent | Split-Path -Parent
  }
  catch
  {
    if(Test-Path "C:\Qt\5.6\mingw49_32\bin\qmake.exe"){
      $Env:QT_BASE_DIR = "C:\Qt\5.6\mingw49_32"
    }
    else
    {
      $Env:QT_BASE_DIR = "C:\Qt\5.6.3\mingw49_32"
    }
  }
}
Write-Output "Using $Env:QT_BASE_DIR as QT base directory." | Tee-Object -File "$logFile" -Append

if(!(Test-Path Env:MINGW_BASE_DIR)){
  $tmp = $Env:QT_BASE_DIR.Split("\\")
  $tmp[-2] = "Tools"
  $Env:MINGW_BASE_DIR = $tmp -join "\"
}
Write-Output "Using $Env:MINGW_BASE_DIR as MinGW base directory." | Tee-Object -File "$logFile" -Append

if(!(Test-Path Env:MINGW_BASE_DIR_BASH)){
  $Env:MINGW_BASE_DIR_BASH = $Env:MINGW_BASE_DIR -replace "\\", "/" -replace "C:", "/c"
}