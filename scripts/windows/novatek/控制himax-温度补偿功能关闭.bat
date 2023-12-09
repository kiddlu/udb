chcp.com 65001

set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;

udb.exe auth
udb.exe shell himax-w 5 0x05 0x0A 0x05 0x00 0x64

pause
