@echo off
REM Run Colason with Qt DLLs from GMAP.SDK vcpkg_installed
set QT_PATH=C:\Users\junya.sakakitani\source\work\GMAP.SDK\Automation\Cpp\vcpkg_installed\x64-windows
set VCPKG_PATH=C:\Users\junya.sakakitani\source\vcpkg\installed\x64-windows

set PATH=%QT_PATH%\bin;%QT_PATH%\plugins;%VCPKG_PATH%\bin;%PATH%
set QT_PLUGIN_PATH=%QT_PATH%\plugins;%QT_PATH%\Qt6\plugins

cd /d "%~dp0"
start "" "build\default\src\Release\colason.exe" %*
