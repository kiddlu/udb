chcp.com 65001

set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;

udb.exe auth

udb.exe runcmd "(cat /proc/kmsg & logcat -d -v ktime)"

pause