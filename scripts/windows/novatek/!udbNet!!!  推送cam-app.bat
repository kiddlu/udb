@echo off

chcp.com 65001

set CUR_DIR=%~dp0
set PATH=%PATH%;%CUR_DIR%\..\bin\

set IPADDR=10.0.50.176

udb -i %IPADDR% auth

udb -i %IPADDR% push %CUR_DIR%\cam_app /usr/bin/cam_app

udb -i %IPADDR% shell "chmod 777  /usr/bin/cam_app"

udb -i %IPADDR% shell "reboot"

pause
exit /b