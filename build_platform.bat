@echo off

setlocal

set libsdir=C:\dev\shared\libs
set incdir=C:\dev\shared\include

set rootdir=%~dp0
set bindir=%rootdir%bin
set srcdir=%rootdir%src

set copt=/I%incdir% /MDd /nologo /FC /GR- /Z7 /Od /Oi /Ob1

set cwopt=/WX /W4 /wd4201 /wd4100 /wd4189 /wd4505 /wd4127

set lopt=/libpath:%libsdir% /debug /opt:ref /incremental:no /subsystem:console

set llib=sav_lib.lib sou_templates.lib

pushd %bindir%

cl %srcdir%\sou_platform.cpp %copt% %cwopt% /DSAV_DEBUG /link %llib% %lopt%

popd

if %errorlevel% neq 0 (exit /b %errorlevel%)
