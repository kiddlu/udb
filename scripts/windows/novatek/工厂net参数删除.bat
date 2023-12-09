chcp.com 65001

set CUR_DIR=%~dp0

set PATH=%PATH%;%CUR_DIR%\..\bin\

udb.exe auth
udb.exe shell rm /fac/net_params.bin

pause