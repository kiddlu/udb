@echo off

chcp.com 65001

set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;
echo persist.dev.slave.mode
udb.exe shell getprop persist.dev.slave.mode

pause