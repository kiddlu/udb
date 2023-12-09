chcp.com 65001

set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;

::         format  min        max
set params=0x00    0x12 0x80  0xff 0x00

for /f "delims=" %%f in ('checksum.exe 0x6e 0xa1 0x14 0x09 %params%') do (set checksum=%%f)

udb.exe shell himax-w 9 0xa1 0x14 0x09 %params% %checksum%

pause
