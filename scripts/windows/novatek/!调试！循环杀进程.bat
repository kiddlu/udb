@echo off

set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;%PATH%


set time=0

:loop
set /a time=time+1

C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe Start-Process -FilePath "%CUR_DIR%..\bin\amcap.exe"
::C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe Start-Process -FilePath "%CUR_DIR%..\bin\amcap.exe" -Windowstyle Hidden

::C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe Start-Process -FilePath "C:\Users\kiddlu\Desktop\HawkMixColorDepth.exe"

busybox.exe sleep 1

busybox.exe ps | busybox grep amcap | busybox awk "{print $1}" | busybox sed "s/^/\/&/g" | busybox xargs nircmd killprocess

echo .... %time% loops

goto :loop