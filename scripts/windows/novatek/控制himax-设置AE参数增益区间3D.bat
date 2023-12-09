chcp.com 65001

set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;

::         min          max
set params=0x10 0x00    0x10 0x00
:: 1-4

for /f "delims=" %%f in ('checksum.exe 0x6e 0x08 0x0F 0x08 %params%') do (set checksum=%%f)

udb.exe shell himax-w 8 0x08 0x0F 0x08 %params% %checksum%

pause
