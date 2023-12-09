@echo off

set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;

echo "请确认是否烧录参考图，回车键确认，否则请关闭窗口退出"

pause

set GT_PATH=%CUR_DIR%\Groundtruth.bin

udb.exe auth

udb.exe push %GT_PATH% /fac/Groundtruth.bin

udb.exe shell sync

udb.exe runcmd himax-flash g /fac/Groundtruth.bin

udb.exe shell reboot

echo "参考图烧录完成，重启设备"

pause
exit
