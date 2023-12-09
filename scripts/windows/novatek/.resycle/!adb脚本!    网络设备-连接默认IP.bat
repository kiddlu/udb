@echo off

chcp.com 65001

set CUR_DIR=%~dp0
set PATH=%PATH%;%CUR_DIR%\..\bin\

::adb kill-server
set IPADDR=10.0.50.176

busybox.exe echo -n -e adbo\x00 | nc.exe %IPADDR% 5164 -u -w 1
adb connect %IPADDR% & adb -s %IPADDR%:5555 shell auth: & adb -s %IPADDR%:5555 shell echo %IPADDR% Connected!
::adb connect %IPADDR%

pause

exit /b