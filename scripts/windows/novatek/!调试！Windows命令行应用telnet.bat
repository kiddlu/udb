@echo off

chcp.com 65001

set CUR_DIR=%~dp0
set PATH=%PATH%;%CUR_DIR%\..\bin\

::sudo dism /online /Enable-Feature /FeatureName:TFTP
sudo dism /online /Enable-Feature /FeatureName:TelnetClient
::pause
