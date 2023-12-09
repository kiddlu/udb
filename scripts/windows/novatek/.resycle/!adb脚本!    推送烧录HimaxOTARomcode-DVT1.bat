@echo off

chcp.com 65001

set CUR_DIR=%~dp0
set PATH=%PATH%;%CUR_DIR%\..\bin\

udb auth

udb shell setprop ctrl.adb.mode yes

busybox sleep 3


for /f %%i in (' nfd dialog ') do (set OTAFILE=%%i)

adb-push "%OTAFILE%" /tmp/himaxota.bin

::adb shell himax-flash o /tmp/himaxota.bin
adb shell himax-ota -f /tmp/himaxota.bin


adb shell reboot

pause

exit /b