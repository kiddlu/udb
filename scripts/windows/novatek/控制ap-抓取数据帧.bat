@echo off

set CUR_DIR=%~dp0

set PATH=%PATH%;%CUR_DIR%\..\bin\

udb.exe shell setprop ctrl.frame.dump 1
udb.exe shell ls -al /var/frame*

pause