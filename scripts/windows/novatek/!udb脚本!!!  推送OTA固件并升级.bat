@echo off

chcp.com 65001

set CUR_DIR=%~dp0
set PATH=%PATH%;%CUR_DIR%\..\bin\

udb auth

udb push %CUR_DIR%\HawkOTA.bin /root/fwota.bin

udb shell reboot2ota

pause
exit /b