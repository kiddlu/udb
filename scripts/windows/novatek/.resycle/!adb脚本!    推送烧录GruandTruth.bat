@echo off

chcp.com 65001

set CUR_DIR=%~dp0
set PATH=%PATH%;%CUR_DIR%\..\bin\

udb auth

udb shell setprop ctrl.adb.mode yes

busybox sleep 3

adb-push %CUR_DIR%\Groundtruth.bin /fac/Groundtruth.bin

adb shell himax-flash g /fac/Groundtruth.bin

adb shell reboot

pause
exit /b