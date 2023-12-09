chcp.com 65001

set CUR_DIR=%~dp0

set PATH=%PATH%;%CUR_DIR%\..\bin\

udb.exe pull ar_params.bin /fac/ar_params.bin
udb.exe pull intrinsic_params.bin /fac/intrinsic_params.bin
udb.exe pull net_params.bin /fac/intrinsic_params.bin