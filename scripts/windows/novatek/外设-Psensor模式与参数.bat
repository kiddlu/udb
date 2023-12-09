@echo off

chcp.com 65001

set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;

udb.exe shell i2ctransfer -y 2 w1@0x3a 0x81;i2ctransfer -y 2 r1@0x3a
busybox.exe echo 
busybox.exe sleep 1

:loop
udb.exe shell i2ctransfer -y 2 w1@0x3a 0x8c;i2ctransfer -y 2 r1@0x3a
udb.exe shell i2ctransfer -y 2 w1@0x3a 0x8b;i2ctransfer -y 2 r1@0x3a
busybox.exe echo 
busybox.exe sleep 1

goto :loop

pause