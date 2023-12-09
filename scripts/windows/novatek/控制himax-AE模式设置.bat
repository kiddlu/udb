chcp.com 65001

set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;

udb.exe shell i2ctransfer -f -y 2 w5@0x37 0x1 0x2 0x5 0xc8 0xa0

pause