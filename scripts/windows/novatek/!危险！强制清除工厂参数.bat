set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;

echo "请确认是否强制清楚工厂参数，回车键确认，否则请关闭窗口退出"

pause

echo "请再次回车键确认，否则请关闭窗口退出"

pause

udb.exe shell rm /fac/ar_params.bin
udb.exe shell rm /fac/intrinsic_params.bin

udb.exe shell reboot

pause