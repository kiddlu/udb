chcp.com 65001

set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;

::         CL值
set params=0x05

for /f "delims=" %%f in ('checksum.exe 0x6e 0xA1 0x08 0x05 %params%') do (set checksum=%%f)

udb.exe shell himax-w 5 0xA1 0x08 0x05 %params% %checksum%

pause
