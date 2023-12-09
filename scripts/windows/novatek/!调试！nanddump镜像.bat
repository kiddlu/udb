@echo off

set CUR_DIR=%~dp0

set PATH=%SystemRoot%;%SystemRoot%\system32;%CUR_DIR%\..\bin\;

udb.exe shell "echo dfudump > /root/uboot.msg"

udb.exe shell reboot

busybox.exe sleep 20

busybox.exe mkdir %CUR_DIR%\nanddump

busybox.exe rm %CUR_DIR%\nanddump\dump.bin

dfu.exe -U %CUR_DIR%\nanddump\dump.bin

busybox.exe dd if=%CUR_DIR%\nanddump\dump.bin of=%CUR_DIR%\nanddump\dump.01.loader         bs=131072 count=2
busybox.exe dd if=%CUR_DIR%\nanddump\dump.bin of=%CUR_DIR%\nanddump\dump.02.fdt            bs=131072 count=2   skip=2
busybox.exe dd if=%CUR_DIR%\nanddump\dump.bin of=%CUR_DIR%\nanddump\dump.03.fdt.restore    bs=131072 count=2   skip=4
busybox.exe dd if=%CUR_DIR%\nanddump\dump.bin of=%CUR_DIR%\nanddump\dump.04.uboot          bs=131072 count=16  skip=6
busybox.exe dd if=%CUR_DIR%\nanddump\dump.bin of=%CUR_DIR%\nanddump\dump.05.uenv           bs=131072 count=2   skip=22
busybox.exe dd if=%CUR_DIR%\nanddump\dump.bin of=%CUR_DIR%\nanddump\dump.06.linux          bs=131072 count=35  skip=24
busybox.exe dd if=%CUR_DIR%\nanddump\dump.bin of=%CUR_DIR%\nanddump\dump.06.rootfs         bs=131072 skip=59

pause