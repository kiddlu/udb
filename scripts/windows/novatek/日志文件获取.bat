chcp.com 65001

set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;

udb.exe auth

udb.exe shell "kmsg > /tmp/log.txt"
udb.exe pull log.txt /tmp/log.txt
udb.exe shell "rm /tmp/log.txt"
udb.exe pull loginit.txt /tmp/loginit.txt
pause