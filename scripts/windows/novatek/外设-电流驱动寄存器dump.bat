@echo off
chcp.com 65001

set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;

busybox.exe echo -n 0x00:
udb.exe shell i2ctransfer 2 -y -f w1@0x63 0x00;i2ctransfer 2 -y -f r1@0x63
busybox.exe echo
busybox.exe echo -n 0x01:
udb.exe shell i2ctransfer 2 -y -f w1@0x63 0x01;i2ctransfer 2 -y -f r1@0x63
busybox.exe echo
busybox.exe echo -n 0x02:
udb.exe shell i2ctransfer 2 -y -f w1@0x63 0x02;i2ctransfer 2 -y -f r1@0x63
busybox.exe echo
busybox.exe echo -n 0x03:
udb.exe shell i2ctransfer 2 -y -f w1@0x63 0x03;i2ctransfer 2 -y -f r1@0x63
busybox.exe echo
busybox.exe echo -n 0x04:
udb.exe shell i2ctransfer 2 -y -f w1@0x63 0x04;i2ctransfer 2 -y -f r1@0x63
busybox.exe echo
busybox.exe echo -n 0x05:
udb.exe shell i2ctransfer 2 -y -f w1@0x63 0x05;i2ctransfer 2 -y -f r1@0x63
busybox.exe echo
busybox.exe echo -n 0x06:
udb.exe shell i2ctransfer 2 -y -f w1@0x63 0x06;i2ctransfer 2 -y -f r1@0x63
busybox.exe echo
busybox.exe echo -n 0x07:
udb.exe shell i2ctransfer 2 -y -f w1@0x63 0x07;i2ctransfer 2 -y -f r1@0x63
busybox.exe echo
busybox.exe echo -n 0x08:
udb.exe shell i2ctransfer 2 -y -f w1@0x63 0x08;i2ctransfer 2 -y -f r1@0x63
busybox.exe echo
busybox.exe echo -n 0x0a:
udb.exe shell i2ctransfer 2 -y -f w1@0x63 0x0a;i2ctransfer 2 -y -f r1@0x63
busybox.exe echo
busybox.exe echo -n 0x0b:
udb.exe shell i2ctransfer 2 -y -f w1@0x63 0x0b;i2ctransfer 2 -y -f r1@0x63
busybox.exe echo
busybox.exe echo -n 0x0c:
udb.exe shell i2ctransfer 2 -y -f w1@0x63 0x0c;i2ctransfer 2 -y -f r1@0x63
busybox.exe echo
busybox.exe echo -n 0x0d:
udb.exe shell i2ctransfer 2 -y -f w1@0x63 0x0d;i2ctransfer 2 -y -f r1@0x63

busybox.exe echo
pause