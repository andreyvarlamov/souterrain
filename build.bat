@echo off

setlocal

set rootdir=%~dp0

call %rootdir%build_lib.bat

call %rootdir%build_game.bat

call %rootdir%build_platform.bat
