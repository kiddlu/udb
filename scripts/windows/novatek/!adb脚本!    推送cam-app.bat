@echo off

chcp.com 65001

set CUR_DIR=%~dp0
set PATH=%PATH%;%CUR_DIR%\..\bin\

udb auth

udb shell setprop ctrl.adb.mode yes

busybox sleep 3

adb-push %CUR_DIR%\cam_app /usr/bin/cam_app

adb shell "chmod 777  /usr/bin/cam_app"

adb shell "reboot"

pause
exit /b