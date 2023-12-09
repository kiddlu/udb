echo off
set CUR_DIR=%~dp0

set PATH=%PATH%;%CUR_DIR%\..\bin\

set DEF_IP=192.168.10.8

::for /f "tokens=*" %%i in (' nfd.exe dialog ') do (set PUSHFILE=%%i)
set PUSHFILE=HawkOTA.bin

tftp.exe  -i -b16384 -p6969 %DEF_IP% PUT %PUSHFILE%  fwota.bin

echo Wait

busybox sleep 30

ping %DEF_IP%

echo Upgrade!!!!
pause
exit
