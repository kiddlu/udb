set CUR_DIR=%~dp0

chcp.com 65001

set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;

udb auth

udb.exe shell "setprop persist.dev.uvc.xfer.mode 2"

udb.exe shell reboot

pause
