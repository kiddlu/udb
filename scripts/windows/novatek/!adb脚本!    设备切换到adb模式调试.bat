@echo off

chcp.com 65001

set CUR_DIR=%~dp0
set PATH=%PATH%;%CUR_DIR%\..\bin\

echo "请确认是否强制进入ADB调试模式，回车键确认，否则请关闭窗口退出"

pause

echo "进入adb模式后，无法再回到uvc模式，只能重启，请再次回车键确认，否则请关闭窗口退出"

pause

udb auth

::udb rawset 0x42 0x58 0x03 0x00 0x05 0x00 0x00 0x00 0x04 0x00 
udb shell killall ssdpd
udb shell killall udpsvd
udb shell killall udhcpc
udb shell killall telnetd
udb shell killall adbd
udb shell setprop ctrl.adb.mode yes

set time1=0
:loop1
busybox.exe sleep 1
set /a time1=time1+1
echo .... %time1% seconds
lsusbdev | busybox.exe grep ADB
if '%time1%' == '5' goto next
if errorlevel 1 goto :loop1

adb shell auth:

pause