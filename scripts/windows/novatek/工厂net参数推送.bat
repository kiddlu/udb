chcp.com 65001

set CUR_DIR=%~dp0

set PATH=%PATH%;%CUR_DIR%\..\bin\

udb.exe auth
udb.exe push net_params.bin /fac/net_params.bin

pause