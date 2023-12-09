echo off
set CUR_DIR=%~dp0

set PATH=%PATH%;%CUR_DIR%\..\bin\


set HOSTTMP=%TMP%

busybox rm -rf %HOSTTMP%\upnp-tmp-dir\
busybox mkdir %HOSTTMP%\upnp-tmp-dir\

for /f "delims=" %%i in (' ipconfig.exe ^| busybox grep IPv4 ^| busybox awk "{print $14}" ') do (listdevices.exe -m %%i >> %HOSTTMP%\upnp-tmp-dir\tmp.txt)

busybox cat %HOSTTMP%\upnp-tmp-dir\tmp.txt | busybox grep berxel | busybox awk "{print $1}" | busybox uniq | busybox awk -F: "{print $2}" | busybox tr -d "/" | clip.exe


for /f "tokens=*" %%i in (' nfd.exe dialog ') do (set PUSHFILE=%%i)

for /f "delims=" %%f in (' wpaste.exe ') do ( tftp.exe  -i -b16384 -p6969 %%f PUT %PUSHFILE%  fwota.bin )

echo Wait

echo Upgrade!!!!
pause
exit
