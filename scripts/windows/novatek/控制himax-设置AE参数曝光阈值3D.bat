@echo on

chcp.com 65001

set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;

::         fmean        bound
set params=0x5a 0x00    0x08 0x00

for /f "delims=" %%f in ('checksum.exe 0x6e 0x08 0x2c 0x08 %params%') do (set checksum=%%f)

udb.exe shell himax-w 8 0x08 0x2c 0x08 %params% %checksum%

pause
