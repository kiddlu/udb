chcp.com 65001

set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;

udb.exe shell i2ctransfer -f -y 2 w4@0x37 0x05 0x02 0x04 0x6D
udb.exe shell i2ctransfer -f -y 2 r6@0x37

pause
