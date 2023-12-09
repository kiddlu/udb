@echo off
chcp.com 65001

set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;

udb.exe shell i2ctransfer 2 -y -f w2@0x63 0x01 0xa7
udb.exe shell i2ctransfer 2 -y -f w2@0x63 0x03 0x7f
udb.exe shell i2ctransfer 2 -y -f w2@0x63 0x04 0x7f
busybox.exe echo
pause