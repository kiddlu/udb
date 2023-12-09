set CUR_DIR=%~dp0

chcp.com 65001

set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;

::1110:0x1e->30
::1100:0x1c->28
::1010:0x1a->26
::1000:0x18->24
::0110:0x16->22
::0100:0x14->20
::0010:0x12->18
::0000:0x10->16
set value=0x1e

udb auth

udb.exe shell devmem 0xF0601018 8

udb.exe shell devmem 0xF0601018 8 %value%

udb.exe shell devmem 0xF0601018 8

pause