set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;

echo "请确认是否强制进入USB下载模式，回车键确认，否则请关闭窗口退出"

pause

echo "请再次回车键确认，否则请关闭窗口退出"

pause

udb.exe auth

udb.exe shell flash_eraseall /dev/mtd0
udb.exe shell reboot

pause