@echo off
chcp.com 65001

set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;


:loop
udb.exe shell cat /sys/bus/iio/devices/iio\:device0/in_voltage0_raw
busybox.exe echo 
udb.exe shell cat /sys/bus/iio/devices/iio\:device0/in_voltage1_raw
busybox.exe echo 
udb.exe shell cat /sys/bus/iio/devices/iio\:device0/in_voltage2_raw
busybox.exe echo 

udb.exe shell cat /sys/devices/virtual/thermal/thermal_zone0/temp
busybox.exe echo 

busybox.exe echo  ===================================================

busybox.exe sleep 1

goto :loop


pause