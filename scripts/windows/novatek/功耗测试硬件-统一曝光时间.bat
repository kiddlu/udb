@echo on

set CUR_DIR=%~dp0

set PATH=%PATH%;%CUR_DIR%\..\bin\


udb.exe auth

udb.exe shell i2ctransfer -f -y 2 w5@0x37 0x1 0x2 0x5 0xc8 0xa0

::         min  mid  max
set params=0x7f 0x7f 0x7f

for /f "delims=" %%f in ('checksum.exe 0x6e 0x08 0x24 0x07 %params%') do (set checksum=%%f)

udb.exe shell himax-w 7 0x08 0x24 0x07 %params% %checksum%


::         min  mid  max
set params=0x7f 0x7f 0x7f

for /f "delims=" %%f in ('checksum.exe 0x6e 0x08 0x26 0x07 %params%') do (set checksum=%%f)

udb.exe shell himax-w 7 0x08 0x26 0x07 %params% %checksum%

pause


::Exposure/Strobe width Value
::0.1ms (min)           0x03
::0.5ms                 0x0F
::1ms                   0x1D
::1.5ms                 0x2C
::2ms                   0x3A
::2.5ms                 0x49
::3ms                   0x57
::3.5ms                 0x66
::4ms                   0x74
::4.5ms (max)           0x83