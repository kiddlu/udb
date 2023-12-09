chcp.com 65001

set CUR_DIR=%~dp0

set PATH=%PATH%;%CUR_DIR%\..\bin\

udb.exe push ar_params.bin /fac/ar_params.bin
udb.exe push intrinsic_params.bin /fac/intrinsic_params.bin

pause