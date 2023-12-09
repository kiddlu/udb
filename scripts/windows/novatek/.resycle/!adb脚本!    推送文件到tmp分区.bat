@echo off

chcp.com 65001

set CUR_DIR=%~dp0
set PATH=%PATH%;%CUR_DIR%\..\bin\

udb auth

udb shell setprop ctrl.adb.mode yes

busybox sleep 3

::adb-push %CUR_DIR%\file /tmp/file

for /f "tokens=*" %%i in (' nfd dialog ') do (set PUSHFILE=%%i)

for /f "tokens=*" %%i in (' echo %PUSHFILE% ') do (set PUSHFILENAME=%%~nxi)

adb-push "%PUSHFILE%" "/tmp/%PUSHFILENAME%"

pause

exit /b
