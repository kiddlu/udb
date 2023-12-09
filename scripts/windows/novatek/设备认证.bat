@echo off

chcp.com 65001

set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;

lsuvc.exe | busybox.exe grep Berxel
if errorlevel 1 goto :nodev

udb.exe auth

pause
exit

:nodev
chcp.com 65001
echo "找不到设备"
pause
exit