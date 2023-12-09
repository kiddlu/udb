@echo off

set CUR_DIR=%~dp0

MSBuild.exe -m /P:Configuration=Release /p:Platform=x86 %CUR_DIR%\build\

copy /y %CUR_DIR%\build\Release\udb.exe %CUR_DIR%\..\scripts\windows\bin\udb.exe

