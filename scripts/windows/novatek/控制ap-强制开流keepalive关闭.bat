set CUR_DIR=%~dp0

set PATH=%PATH%;%CUR_DIR%\..\bin\

udb.exe shell setprop ctrl.force.keepalive no

pause