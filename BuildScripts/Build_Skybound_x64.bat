@echo off
rem running UE4Editor.exe, please hold...
pushd "%~dp0\..
.\Engine\Build\BatchFiles\Build.bat Skybound Win64 Development rem _Editor

pause