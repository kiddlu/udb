echo off
chcp.com 65001

set CUR_DIR=%~dp0
set PATH=%PATH%;%CUR_DIR%\..\bin\

udb.exe auth

::udb.exe shell fw_setenv dfumode    yes
::udb.exe shell fw_setenv dfu-himaxota  no

udb.exe shell "echo dfumode > /root/uboot.msg"

udb.exe shell reboot


set time1=0
:loop1
busybox.exe sleep 1
set /a time1=time1+1
echo .... %time1% seconds
lsusbdev.exe | busybox.exe grep DFU
if '%time1%' == '7' goto next
if errorlevel 1 goto :loop1

:next
for /f "tokens=*" %%i in (' nfd.exe dialog ') do (set PUSHFILE=%%i)

dfu.exe -D %PUSHFILE%

echo Wait

set time2=0
:loop2
busybox.exe sleep 5
set /a time2=time2+5
echo .... %time2% seconds
lsuvc.exe | busybox.exe grep Berxel
if '%time1%' == '250' goto fail
if errorlevel 1 goto :loop2

echo Upgrade!!!!
udb.exe auth
udb.exe shell getprop | busybox.exe grep -e version -e romcode -e boot.ver
pause
exit

:fail
echo Failed!!!!
pause
exit