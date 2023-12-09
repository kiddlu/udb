chcp.com 65001

set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;

udb.exe shell i2ctransfer -f -y 2 w4@0x37 0xA1 0x02 0x04 0xC9
udb.exe shell i2ctransfer -f -y 2 r6@0x37

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