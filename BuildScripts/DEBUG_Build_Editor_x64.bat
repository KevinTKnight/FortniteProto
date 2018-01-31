@echo off
rem running UE4Editor.exe, please hold...
pushd "%~dp0\..
.\Engine\Build\BatchFiles\Build.bat UE4Editor Win64 Debug rem _Editor

pause