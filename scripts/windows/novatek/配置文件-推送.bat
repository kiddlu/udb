chcp.com 65001

set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;

udb.exe push cam_app.ini /etc/cam_app.ini
udb.exe shell sync

echo 请重启设备

pause