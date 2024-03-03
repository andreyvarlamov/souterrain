@echo off

setlocal

set rootdir=%~dp0
set bindir=%rootdir%bin

pushd %rootdir%

%bindir%\sou_platform.exe

popd
