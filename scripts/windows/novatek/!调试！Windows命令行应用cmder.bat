@echo on

chcp.com 65001

set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;

busybox rm -rf %~dp0/../bin/cmder_mini

busybox unzip %~dp0/../bin/cmder_mini.zip -d %~dp0/../bin/cmder_mini

start %~dp0/../bin/cmder_mini/Cmder.exe