@echo off
set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;

udb.exe shell fw_setenv skip_ota_header_check 1

pause