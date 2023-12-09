@echo off
chcp.com 65001

set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;


::===================================================================
::set PID=0x0002
::
::udb auth
::udb shell sed -i "s/pid=0x[^*]*[0-9A-Z]/pid=%PID%/g" /etc/cam_app.ini
::udb shell reboot
::pause