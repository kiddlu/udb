@echo off
chcp.com 65001

set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;
udb.exe auth

::set boltage
udb.exe shell setprop ctrl.dac.set.tx 1250

::enable tec gpio
udb.exe shell gpio 234 1

busybox.exe echo
pause