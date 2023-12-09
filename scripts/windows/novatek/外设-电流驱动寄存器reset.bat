@echo off
chcp.com 65001

set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;
udb.exe shell i2ctransfer 2 -y -f w2@0x63 0x07 0x80
busybox.exe echo
pause