@echo off

chcp.com 65001

set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;

udb.exe rawset 0x42 0x58 0x03 0x00 0x05 0x00 0x00 0x00 0x10 0x00 ^
                                                                           ^
           0x00

echo "设置完毕，请等待重启"

pause