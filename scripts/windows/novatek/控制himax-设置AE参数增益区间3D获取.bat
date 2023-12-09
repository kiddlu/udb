chcp.com 65001

set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;


udb.exe shell i2ctransfer -f -y 2 w4@0x37 0x08 0x10 0x04 0x72
udb.exe shell i2ctransfer -f -y 2 r9@0x37
pause
