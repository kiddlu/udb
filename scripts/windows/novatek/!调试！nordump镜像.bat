@echo on

chcp.com 65001

set CUR_DIR=%~dp0
set PATH=%PATH%;%CUR_DIR%\..\bin\

udb.exe auth
udb.exe shell setprop ctrl.adb.mode yes

busybox sleep 4

adb.exe shell auth:
for /f "tokens=*" %%i in (' adb shell cat /etc/sn.txt ') do (set DEVSN=%%i)
adb.exe shell "himax-flash r /tmp/dump.bin"
busybox mkdir %CUR_DIR%\nordump-%DEVSN%\
adb.exe pull /tmp/dump.bin %CUR_DIR%\nordump-%DEVSN%\dump.bin

busybox.exe dd if=%CUR_DIR%\nordump-%DEVSN%\dump.bin of=%CUR_DIR%\nordump-%DEVSN%\dump.01.fw        bs=262144 count=2
busybox.exe dd if=%CUR_DIR%\nordump-%DEVSN%\dump.bin of=%CUR_DIR%\nordump-%DEVSN%\dump.02.gt        bs=262144 count=3   skip=2
busybox.exe dd if=%CUR_DIR%\nordump-%DEVSN%\dump.bin of=%CUR_DIR%\nordump-%DEVSN%\dump.03.ota       bs=262144 count=2   skip=5

pause