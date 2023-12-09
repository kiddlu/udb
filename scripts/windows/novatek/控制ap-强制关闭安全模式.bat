set CUR_DIR=%~dp0

chcp.com 65001

set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;

udb.exe shell setprop ctrl.ps.safemode no

pause