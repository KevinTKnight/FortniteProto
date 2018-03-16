@echo off
rem running UE4Editor.exe, please hold...
pushd "%~dp0\..
.\Engine\Build\BatchFiles\Rebuild.bat Skybound Win64 Development rem _Editor

pause