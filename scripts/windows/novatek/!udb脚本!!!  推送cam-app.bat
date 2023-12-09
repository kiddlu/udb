@echo off

chcp.com 65001

set CUR_DIR=%~dp0
set PATH=%PATH%;%CUR_DIR%\..\bin\

udb auth

udb push %CUR_DIR%\cam_app /usr/bin/cam_app

udb shell "chmod 777  /usr/bin/cam_app"

udb shell "reboot"

pause
exit /b