chcp.com 65001

set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;

::         温度
set params=0x1C

for /f "delims=" %%f in ('checksum.exe 0x6e 0x05 0x0B 0x05 %params%') do (set checksum=%%f)

udb.exe shell himax-w 5 0x05 0x0B 0x05 %params% %checksum%

pause
