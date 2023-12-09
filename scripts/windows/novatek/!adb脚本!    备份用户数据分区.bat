@echo on

chcp.com 65001

set CUR_DIR=%~dp0
set PATH=%PATH%;%CUR_DIR%\..\bin\

udb auth

udb shell setprop ctrl.adb.mode yes

busybox sleep 5

adb shell auth:


for /f "tokens=*" %%i in (' adb shell hostname ') do (set DEVSN=%%i)

adb pull /fac/ %CUR_DIR%/fac-%DEVSN%/

pause

exit /b
