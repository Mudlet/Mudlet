Set-Variable -Name "uri" -Value "https://make.mudlet.org/snapshots/Mudlet-$env:VERSION$env:MUDLET_VERSION_BUILD-windows.zip"
Set-Variable -Name "inFile" -Value "Mudlet-$env:VERSION$env:MUDLET_VERSION_BUILD-windows.zip"
Set-Variable -Name "outFile" -Value "upload-result.txt"
Invoke-RestMethod -Uri $uri -Method PUT -InFile $inFile -OutFile $outFile
Get-Content -Path $outFile
