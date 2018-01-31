@echo off
rem running UE4Editor.exe, please hold...
pushd "%~dp0\..
.\Engine\Build\BatchFiles\Rebuild.bat UE4Editor Win64 Development rem _Editor

pause