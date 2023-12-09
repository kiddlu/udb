@echo off
chcp.com 65001

set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;


::===================================================================
set SWITCH=1

udb auth
udb shell wcfg thermal_defstate %SWITCH%
udb shell reboot
pause