@echo off
rem DOS pkzip.exe does not work under WinXP !
rem instead using infozip zip.exe

IF EXIST zzipselftest%1.zip del zzipselftest%1.zip
echo zip -0 zzipselftest%1.zip zzipself%1.exe 
     zip -0 zzipselftest%1.zip zzipself%1.exe
echo zip -9 zzipselftest%1.zip zzipself.txt
     zip -9 zzipselftest%1.zip zzipself.txt
echo zzipsetstub%1.exe zzipselftest%1.zip zzipself%1.exe
     zzipsetstub%1.exe zzipselftest%1.zip zzipself%1.exe
echo  rename zzipselftest%1.zip zzipselftest%1.exe
IF EXIST zzipselftest%1.exe del zzipselftest%1.exe
      ren    zzipselftest%1.zip zzipselftest%1.exe

REM testrun:
echo now testing zzipselftest%1.exe
zzipselftest%1.exe zzipself.txt
