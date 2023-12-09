@echo off
chcp.com 65001

set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;
udb.exe auth

udb.exe shell setprop ctrl.tec.set.temp 35

udb.exe shell setprop ctrl.tec.func.en  no

busybox.exe echo
pause