@echo off

set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;

echo "请确认是否烧录参数，回车键确认，否则请关闭窗口退出"

pause

set AR_PATH=%CUR_DIR%\fac-HK100HA21P100013\ar_params.bin
set IN_PATH=%CUR_DIR%\fac-HK100HA21P100013\intrinsic_params.bin

udb.exe auth

udb.exe push %AR_PATH% /fac/ar_params.bin

udb.exe push %IN_PATH% /fac/intrinsic_params.bin

udb.exe shell sync

udb.exe shell reboot

echo "参数烧录完成，重启设备"

pause
exit
